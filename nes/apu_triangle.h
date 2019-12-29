#ifndef PROTONES_NES_APU_TRIANGLE_H
#define PROTONES_NES_APU_TRIANGLE_H
#include <cstdint>
#include "nes/base.h"
#include "nes/nes.h"
#include "proto/apu.pb.h"
namespace protones {

class Triangle: public APUDevice {
  public:
    Triangle(NES* nes);

    Type type() const override { return Type::Triangle; }
    float Output();
    void StepTimer();
    void StepLength();
    void StepCounter();

    void set_enabled(bool val);
    void set_control(uint8_t val);
    void set_timer_low(uint8_t val);
    void set_timer_high(uint8_t val);
    inline uint16_t length() const { return length_value_; }
    void LoadState(proto::APUTriangle *state);
    void SaveState(proto::APUTriangle *state);

  private:
    uint8_t InternalOutput();
    NES* nes_;
    static const int channel_ = 2;
    bool enabled_;

    bool length_enabled_;
    uint8_t length_value_;

    uint16_t timer_period_;
    uint16_t timer_value_;

    uint8_t duty_value_;

    bool counter_reload_;
    uint8_t counter_period_;
    uint8_t counter_value_;

    struct {
        uint8_t control, tlo, thi;
    } reg_;
    friend class APUDebug;
};

}  // namespace protones
#endif // PROTONES_NES_APU_TRIANGLE_H
