#ifndef PROTONES_NES_MIDI_H
#define PROTONES_NES_MIDI_H
#include <cstdint>
#include <cmath>
#include <memory>

#include "nes/base.h"
#include "RtMidi.h"

// The midi connector isn't really a part of a NES, but a fun toy for
// playing with the music events from the NES.
namespace protones {
class MidiConnector : public EmulatedDevice {
  public:
    MidiConnector()
      : enabled_(false),
      channel_{},
      chan_volume_{1.0, 1.0, 1.0, 1.0},
      midi_(new RtMidiOut)
    { InitFreqTable(440.0); }
    struct Note {
        uint8_t note;
        uint8_t velocity;
    };

    void NoteOnFreq(uint8_t chan, double f, uint8_t velocity);
    void NoteOn(uint8_t chan, uint8_t note, uint8_t velocity);
    void NoteOff(uint8_t chan);

    bool enabled() { return enabled_; }

    float& chan_volume(int i) { return chan_volume_[i]; }
    void set_enabled(bool val) { enabled_ = val; }
    inline double frequency(double t) {
        return CPU / (16.0 * (t + 1));
    }
    RtMidiOut* out() { return midi_.get(); }

    void InitFreqTable(double a4);
    int FindFreqIndex(double f);
    static constexpr int NCHAN = 4;
  private:
    bool enabled_;
    Note channel_[NCHAN];
    float chan_volume_[NCHAN];
    std::unique_ptr<RtMidiOut> midi_;
    double freq_[128]; 
    static const char* notes_[128];
    static constexpr double TRT = std::pow(2.0, 1.0/12.0);
    static constexpr double CPU = 1789773;
};
}  // namespace protones

#endif // PROTONES_NES_MIDI_H
