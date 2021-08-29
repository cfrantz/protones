#ifndef PROTONES_MIDI_MIDI_H
#define PROTONES_MIDI_MIDI_H
#include <stdint.h>
#include <cmath>
#include <memory>

#include "google/protobuf/text_format.h"
#include "nes/base.h"
#include "nes/nes.h"
#include "proto/fti.pb.h"
#include "proto/midi.pb.h"
#include "RtMidi.h"

// The midi connector isn't really a part of a NES, but a fun toy for
// playing with the music events from the NES.
namespace protones {

class Envelope {
  public:
    static constexpr int STATE_OFF = 0;
    static constexpr int STATE_ON = 1;
    static constexpr int STATE_RELEASE = 2;

    Envelope(const proto::Envelope* e)
      : envelope_(e),
      state_(STATE_OFF),
      frame_(0),
      default_(0),
      value_(0)
      {}

    Envelope(int8_t def)
      : envelope_(nullptr),
      state_(STATE_OFF),
      frame_(0),
      default_(def),
      value_(0)
      {}

    Envelope()
      : envelope_(nullptr),
      state_(STATE_OFF),
      frame_(0),
      default_(0),
      value_(0)
      {}

    int8_t value();
    void Reset(int state);
    void NoteOn();
    void Release();
    void Step();

    bool done() { return state_ == STATE_OFF; }
  private:
    const proto::Envelope* envelope_;
    int state_;
    int frame_;
    int8_t default_;
    int8_t value_;
};

class InstrumentPlayer {
  public:
    InstrumentPlayer(proto::FTInstrument *inst)
      : instrument_(inst),
      note_(0),
      velocity_(0)
    {
        if (instrument_ != nullptr) {
            volume_ = Envelope(&instrument_->volume());
            arpeggio_ = Envelope(&instrument_->arpeggio());
            pitch_ = Envelope(&instrument_->pitch());
            duty_ = Envelope(&instrument_->duty());
        }
    }

    void NoteOn(uint8_t note, uint8_t velocity);
    void Release();
    void Step();
    uint8_t volume();
    uint8_t duty();
    uint16_t timer();

    uint8_t note() { return note_; }
    bool done();
  private:
    proto::FTInstrument *instrument_;
    uint8_t note_;
    uint8_t velocity_;
    Envelope volume_ = Envelope(15);
    Envelope arpeggio_ = Envelope();
    Envelope pitch_ = Envelope();
    Envelope duty_ = Envelope(2);
};


class Channel {
  public:
    Channel(NES* nes, const proto::MidiChannel& config)
      : nes_(nes),
      config_(config)
    {}
    
    void ProcessMessage(const std::vector<uint8_t>& message);
    void Step();
    void NoteOn(uint8_t note, uint8_t velocity);
    void NoteOff(uint8_t note);
  private:
    uint16_t OscBaseAddress(proto::MidiChannel::Oscillator osciallator);

    NES* nes_; 
    proto::MidiChannel config_;
    std::vector<InstrumentPlayer> player_;
    std::map<uint16_t, uint8_t> last_timer_hi_;
}; 


class MidiConnector : public EmulatedDevice {
  public:
    MidiConnector(NES* nes)
      : nes_(nes),
      enabled_(false),
      midi_(new RtMidiIn)
    {
        MidiConnector::InitNotes(440.0);
    }

    bool enabled() { return enabled_; }
    void set_enabled(bool val) {
        enabled_ = val;
        InitEnables();
    }

    RtMidiIn* midi() { return midi_.get(); }

    void Emulate();
    void ProcessMessages();
    void ProcessMessage(const std::vector<uint8_t>& message);
    void LoadConfig(const std::string& filename);

    static void InitNotes(double a440);
    static int notes_[128];
  private:
    void InitEnables();
    NES* nes_; 
    bool enabled_;
    std::unique_ptr<RtMidiIn> midi_;
    proto::MidiConfig config_;
    std::map<std::string, std::unique_ptr<Channel>> channel_;

    static constexpr double TRT = std::pow(2.0, 1.0/12.0);
};

}  // namespace protones

#endif // PROTONES_MIDI_MIDI_H
