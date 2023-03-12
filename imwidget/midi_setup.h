#ifndef PROTONES_IMWIDGET_MIDI_SETUP_H
#define PROTONES_IMWIDGET_MIDI_SETUP_H
#include <string>
#include <vector>

#include "imwidget/imwidget.h"
#include "midi/midi.h"
#include "proto/fti.pb.h"

namespace protones {

class NES;

struct VRC7Patch {
    bool is_modulator;
    bool tremolo;
    bool vibrato;
    bool sustain;
    bool key_rate;
    int multiplier;
    int key_scaling;
    int output;
    bool waveform;
    int feedback;
    int adsr[4];

    static VRC7Patch Modulator(uint8_t regs[8]) {
        VRC7Patch patch{};
        patch.is_modulator = true;
        patch.tremolo =  (regs[0] & 0x80) != 0;
        patch.vibrato =  (regs[0] & 0x40) != 0;
        patch.sustain =  (regs[0] & 0x20) != 0;
        patch.key_rate = (regs[0] & 0x10) != 0;
        patch.multiplier = regs[0] & 0x0F;
        patch.key_scaling = regs[2] >> 6;
        patch.output = regs[2] & 0x3F;
        patch.waveform = (regs[3] & 0x08) != 0;
        patch.feedback = regs[3] & 0x07;
        patch.adsr[0] = regs[4] >> 4;
        patch.adsr[1] = regs[4] & 0xF;
        patch.adsr[2] = regs[6] >> 4;
        patch.adsr[3] = regs[6] & 0xF;
        return patch;
    }
    static VRC7Patch Carrier(uint8_t regs[8]) {
        VRC7Patch patch{};
        patch.is_modulator = false;
        patch.tremolo =  (regs[1] & 0x80) != 0;
        patch.vibrato =  (regs[1] & 0x40) != 0;
        patch.sustain =  (regs[1] & 0x20) != 0;
        patch.key_rate = (regs[1] & 0x10) != 0;
        patch.multiplier = regs[1] & 0x0F;
        patch.key_scaling = regs[3] >> 6;
        patch.waveform = (regs[3] & 0x10) != 0;
        patch.adsr[0] = regs[5] >> 4;
        patch.adsr[1] = regs[5] & 0xF;
        patch.adsr[2] = regs[7] >> 4;
        patch.adsr[3] = regs[7] & 0xF;
        return patch;
    }
    void ToRegs(uint8_t regs[8]) {
        if (is_modulator) {
            regs[0] = multiplier & 0x0F;
            regs[0] |= tremolo ? 0x80 : 0;
            regs[0] |= vibrato ? 0x40 : 0;
            regs[0] |= sustain ? 0x20 : 0;
            regs[0] |= key_rate ? 0x10 : 0;
            regs[2] = key_scaling << 6;
            regs[2] |= output & 0x3f;
            regs[3] &= 0xF0;
            regs[3] |= waveform ? 0x08 : 0;
            regs[3] |= feedback & 0x07;
            regs[4] = adsr[0] << 4;
            regs[4] |= adsr[1] & 0x0F;
            regs[6] = adsr[2] << 4;
            regs[6] |= adsr[3] & 0x0F;
        } else {
            regs[1] = multiplier & 0x0F;
            regs[1] |= tremolo ? 0x80 : 0;
            regs[1] |= vibrato ? 0x40 : 0;
            regs[1] |= sustain ? 0x20 : 0;
            regs[1] |= key_rate ? 0x10 : 0;
            regs[3] &= 0x0F;
            regs[3] |= key_scaling << 6;
            regs[3] |= waveform ? 0x10 : 0;
            regs[5] = adsr[0] << 4;
            regs[5] |= adsr[1] & 0x0F;
            regs[7] = adsr[2] << 4;
            regs[7] |= adsr[3] & 0x0F;
        }
    }
};

class MidiSetup: public ImWindowBase {
  public:
    MidiSetup(NES* nes)
      : ImWindowBase(false, false),
      nes_(nes)
    {}

    bool Draw() override;
    void DrawVRC7(proto::FTInstrument* inst);
    void DrawEnvelope(proto::Envelope* envelope, proto::Envelope_Kind kind, InstrumentPlayer *player);

  private:
    void GetPortNames();

    NES* nes_;
    bool enabled_ = false;
    bool ignore_program_change_ = false;
    int ports_ = 0;
    int current_port_ = 0;
    std::vector<std::string> portnames_;
    std::vector<const std::string*> instruments_;
    std::string current_channel_;
    std::string current_instrument_;
    const char* names_[128];
};

}  // namespace
#endif // PROTONES_IMWIDGET_MIDI_SETUP_H
