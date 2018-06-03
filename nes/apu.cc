#include <string.h>
#include <gflags/gflags.h>
#include <SDL2/SDL.h>
#include "imgui.h"

#include "util/os.h"
#include "nes/apu.h"
#include "nes/nes.h"

DEFINE_double(volume, 0.5, "Sound volume");
namespace protones {

static float pulse_table[32];
static float other_table[204];

static void init_tables() {
    static int once;
    int i;
    if (!once) {
        once = 1;
        for(i=0; i<32; i++) {
            pulse_table[i] = 95.52 / (8128/double(i) + 100);
        }
        for(i=0; i<204; i++) {
            other_table[i] = 163.67 / (24329/double(i) + 100);
        }
    }
}

APU::APU(NES *nes)
    : nes_(nes),
    pulse_({1, 2}),
    dmc_(nes),
    cycle_(0),
    frame_period_(0),
    frame_value_(0),
    frame_irq_(0),
    volume_(FLAGS_volume),
    data_{0, },
    len_(0) {
        mutex_ = SDL_CreateMutex();
        cond_ = SDL_CreateCond();
        init_tables();
}

void APU::LoadState(proto::APU* state) {
    pulse_[0].LoadState(state->mutable_pulse(0));
    pulse_[1].LoadState(state->mutable_pulse(1));
    triangle_.LoadState(state->mutable_triangle());
    noise_.LoadState(state->mutable_noise());
    dmc_.LoadState(state->mutable_dmc());
}

void APU::SaveState(proto::APU* state) {
    state->clear_pulse();
    pulse_[0].SaveState(state->add_pulse());
    pulse_[1].SaveState(state->add_pulse());
    triangle_.SaveState(state->mutable_triangle());
    noise_.SaveState(state->mutable_noise());
    dmc_.SaveState(state->mutable_dmc());
}

void APU::StepTimer() {
    if (cycle_ % 2 == 0) {
        pulse_[0].StepTimer();
        pulse_[1].StepTimer();
        noise_.StepTimer();
        dmc_.StepTimer();
    }
    triangle_.StepTimer();
}

void APU::StepEnvelope() {
    pulse_[0].StepEnvelope();
    pulse_[1].StepEnvelope();
    triangle_.StepCounter();
    noise_.StepEnvelope();
}

void APU::StepSweep() {
    pulse_[0].StepSweep();
    pulse_[1].StepSweep();
}

void APU::StepLength() {
    pulse_[0].StepLength();
    pulse_[1].StepLength();
    triangle_.StepLength();
    noise_.StepLength();
}

void APU::SignalIRQ() {
    if (frame_irq_)
        nes_->IRQ();
}

void APU::StepFrameCounter() {
    if (frame_period_ == 4) {
        frame_value_ = (frame_value_ + 1) % 4;
        StepEnvelope();
        if (frame_value_ & 1) {
            StepSweep();
            StepLength();
            if (frame_value_ == 3)
                SignalIRQ();
        }
    } else {
        frame_value_ = (frame_value_ + 1) % 5;
        if (frame_value_ == 4)
            return;
        StepEnvelope();
        if (!(frame_value_ & 1)) {
            StepSweep();
            StepLength();
        }
    }
}

float APU::Output() {
    uint8_t p0 = pulse_[0].Output();
    uint8_t p1 = pulse_[1].Output();
    uint8_t t = triangle_.Output();
    uint8_t n = noise_.Output();
    uint8_t d = dmc_.Output();
    return volume_* (pulse_table[p0+p1] + other_table[t*3 + n*2 + d]);
}

void APU::Emulate() {
    double c1 = double(cycle_);
    double c2 = double(++cycle_);

    StepTimer();

    // Every 7457.38 clocks
    int f1 = int(c1 / NES::frame_counter_rate);
    int f2 = int(c2 / NES::frame_counter_rate);
    if (f1 != f2)
        StepFrameCounter();

    // Every 40.58 clocks
    int s1 = int(c1 / NES::sample_rate);
    int s2 = int(c2 / NES::sample_rate);
    if (s1 != s2) {
#if 0
        SDL_LockMutex(mutex_);
        while(len_ == BUFFERLEN) {
            SDL_CondWait(cond_, mutex_);
        }
        if (len_ < BUFFERLEN) {
            data_[len_++] = Output();
        } else {
            fprintf(stderr, "Audio overrun\n");
        }
        SDL_UnlockMutex(mutex_);
#else
        while((producer_ + 1) % BUFFERLEN == consumer_ % BUFFERLEN) {
            os::SchedulerYield();
        }
        data_[producer_ % BUFFERLEN] = Output();
        ++producer_;
#endif
    }
}

void APU::PlayBuffer(void* stream, int bufsz) {
    int n = bufsz / sizeof(float);
#if 0
    if (len_ >= n) {
        SDL_LockMutex(mutex_);
        int rest = len_ - n;
        memcpy(stream, data_, bufsz);
        memmove(data_, data_ + n, rest * sizeof(float));
        len_ = rest;
        SDL_CondSignal(cond_);
        SDL_UnlockMutex(mutex_);
    } else {
        //fprintf(stderr, "Audio underrun\n");
        memset(stream, 0, bufsz);
    }
#else
    float* out = static_cast<float*>(stream);
    while(n && consumer_ < producer_) {
        *out++ = data_[consumer_ % BUFFERLEN];
        ++consumer_;
        --n;
    }
    while(n) {
        *out++ = 0.0f;
        --n;
    }
#endif
}

void APU::Write(uint16_t addr, uint8_t val) {
    switch(addr) {
    case 0x4000: pulse_[0].set_control(val); break;
    case 0x4001: pulse_[0].set_sweep(val); break;
    case 0x4002: pulse_[0].set_timer_low(val); break;
    case 0x4003: pulse_[0].set_timer_high(val); break;

    case 0x4004: pulse_[1].set_control(val); break;
    case 0x4005: pulse_[1].set_sweep(val); break;
    case 0x4006: pulse_[1].set_timer_low(val); break;
    case 0x4007: pulse_[1].set_timer_high(val); break;

    case 0x4008: triangle_.set_control(val); break;
    case 0x400a: triangle_.set_timer_low(val); break;
    case 0x400b: triangle_.set_timer_high(val); break;

    case 0x400c: noise_.set_control(val); break;
    case 0x400e: noise_.set_period(val); break;
    case 0x400f: noise_.set_length(val); break;

    case 0x4010: dmc_.set_control(val); break;
    case 0x4011: dmc_.set_value(val); break;
    case 0x4012: dmc_.set_address(val); break;
    case 0x4013: dmc_.set_length(val); break;

    case 0x4015: set_control(val); break;
    case 0x4017: set_frame_counter(val); break;

    default:
        ; // Nothing
    }
}

uint8_t APU::Read(uint16_t addr) {
    uint8_t result = 0;
    if (addr == 0x4015) {
        result |= (pulse_[0].length() > 0) << 0;
        result |= (pulse_[1].length() > 0) << 1;
        result |= (triangle_.length() > 0) << 2;
        result |= (noise_.length() > 0   ) << 3;
        result |= (dmc_.length() > 0     ) << 4;
    }
    return result;
}

void APU::set_frame_counter(uint8_t val) {
    frame_period_ = 4 + ((val >> 7) & 1);
    frame_irq_ = !(val & 0x40);
}

void APU::set_control(uint8_t val) {
    pulse_[0].set_enabled(!!(val & 0x01));
    pulse_[1].set_enabled(!!(val & 0x02));
    triangle_.set_enabled(!!(val & 0x04));
    noise_.set_enabled(!!(val & 0x08));
    dmc_.set_enabled(!!(val & 0x10));
}
}  // namespace protones
