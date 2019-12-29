#ifndef PROTONES_NES_APU_DMC_H
#define PROTONES_NES_APU_DMC_H
#include <cstdint>
#include "nes/base.h"
#include "nes/nes.h"
#include "proto/apu.pb.h"

namespace protones {

class DMC: public APUDevice {
  public:
    DMC(NES* nes);
    
    Type type() const override { return Type::DMC; }
    float Output();
    void StepReader();
    void StepShifter();
    void StepTimer();

    void set_enabled(bool val);
    void set_control(uint8_t val);
    void set_value(uint8_t val);
    void set_address(uint8_t val);
    void set_length(uint8_t val);
    inline uint16_t length() const { return current_length_; }
    void restart();
    void LoadState(proto::APUDMC* state);
    void SaveState(proto::APUDMC* state);

  private:
    uint8_t InternalOutput();
    NES* nes_;
    bool enabled_;
    uint8_t value_;

    uint16_t sample_address_;
    uint16_t sample_length_;
    uint16_t current_address_;
    uint16_t current_length_;

    uint8_t shift_register_;
    uint8_t bit_count_;
    uint8_t tick_value_;
    uint8_t tick_period_;

    bool loop_;
    bool irq_;
    struct {
        uint8_t control, value, address, length;
    } reg_;
    friend class APUDebug;
};

}  // namespace protones
#endif // PROTONES_NES_APU_DMC_H
