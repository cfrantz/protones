#ifndef PROTONES_NES_APU_H
#define PROTONES_NES_APU_H
#include <cstdint>
#include <chrono>
#include <atomic>
#include <SDL2/SDL.h>

#include "proto/apu.pb.h"
#include "nes/base.h"
#include "nes/apu_dmc.h"
#include "nes/apu_noise.h"
#include "nes/apu_pulse.h"
#include "nes/apu_triangle.h"
#include "nes/nes.h"

namespace protones {

class APU : public EmulatedDevice {
  public:
    APU(NES* nes);
    ~APU() {}

    void Write(uint16_t addr, uint8_t val);
    uint8_t Read(uint16_t addr);

    void PlayBuffer(void* stream, int len);

    void StepEnvelope();
    void StepLength();
    void StepSweep();
    void StepTimer();
    void StepFrameCounter();
    void SignalIRQ();
    float Output();
    void Emulate();

    void LoadState(proto::APU* state);
    void SaveState(proto::APU* state);
    void set_volume(float v) { volume_ = v; }
    static const int BUFFERLEN = 1024;
  private:
    void set_frame_counter(uint8_t val);
    void set_control(uint8_t val);

    NES* nes_;
    Pulse pulse_[2];
    Triangle triangle_;
    Noise noise_;
    DMC dmc_;
    SDL_mutex *mutex_;
    SDL_cond *cond_;

    uint64_t cycle_;
    uint8_t frame_period_;
    uint8_t frame_value_;;
    bool frame_irq_;
    float volume_;

    float data_[BUFFERLEN];
    std::atomic<int> len_;
    std::atomic<int> producer_, consumer_;
    friend class APUDebug;
};

}  // namespace protones
#endif // PROTONES_NES_APU_H
