#include "imgui.h"
#include "nes/apu_dmc.h"
#include "nes/mem.h"
#include "pbmacro.h"

namespace protones {
static uint8_t dmc_table[16] = {
    214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27,
};

DMC::DMC(NES* nes) :
    nes_(nes),
    enabled_(0),
    value_(0),
    sample_address_(0), sample_length_(0),
    current_address_(0), current_length_(0),
    shift_register_(0), bit_count_(0), tick_value_(0), tick_period_(0),
    loop_(0), irq_(0),
    dbgp_(0) {}

void DMC::SaveState(proto::APUDMC *state) {
    SAVE(enabled,
         value,
         sample_address, sample_length,
         current_address, current_length,
         shift_register, bit_count, tick_value, tick_period,
         loop, irq);
}

void DMC::LoadState(proto::APUDMC *state) {
    LOAD(enabled,
         value,
         sample_address, sample_length,
         current_address, current_length,
         shift_register, bit_count, tick_value, tick_period,
         loop, irq);
}

uint8_t DMC::Output() {
    dbgbuf_[dbgp_] = value_;
    dbgp_ = (dbgp_ + 1) % DBGBUFSZ;
    return value_;
}

void DMC::StepReader() {
    if (current_length_ > 0 && bit_count_ == 0) {
        nes_->Stall(4);
        shift_register_ = nes_->mem()->Read(current_address_);
        bit_count_ = 8;
        current_address_++;
        if (current_address_ == 0)
            current_address_ = 0x8000;
        current_length_--;
        if (current_length_ == 0 && loop_)
            restart();
    }
}

void DMC::StepShifter() {
    if (bit_count_ == 0)
        return;
    if (shift_register_ & 1) {
        if (value_ <= 125)
            value_ += 2;
    } else if (value_ >= 2) {
        value_ -= 2;
    }
    shift_register_ >>= 1;
    bit_count_--;
}

void DMC::StepTimer() {
    if (!enabled_)
        return;
    StepReader();
    if (tick_value_ == 0) {
        tick_value_ = tick_period_;
        StepShifter();
    } else {
        tick_value_--;
    }
}

void DMC::set_enabled(bool val) {
    enabled_ = val;
    if (!enabled_) {
        current_length_ = 0;
    } else if (current_length_ == 0) {
        restart();
    }
}

void DMC::set_control(uint8_t val) {
    reg_.control = val;
    irq_ = !!(val & 0x80);
    loop_ = !!(val & 0x40);
    tick_period_ = dmc_table[val & 0x0f];
}

void DMC::set_value(uint8_t val) {
    reg_.value = val;
    value_ = val & 0x7f;
}

void DMC::set_address(uint8_t val) {
    reg_.address = val;
    sample_address_ = 0xC000 | (uint16_t(val) << 6);
}

void DMC::set_length(uint8_t val) {
    reg_.length = val;
    sample_length_ = (uint16_t(val) << 4) | 1;
}

void DMC::restart() {
    current_address_ = sample_address_;
    current_length_ = sample_length_;
}
}  // namespace protones
