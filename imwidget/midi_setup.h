#ifndef PROTONES_IMWIDGET_MIDI_SETUP_H
#define PROTONES_IMWIDGET_MIDI_SETUP_H
#include <string>
#include <vector>

#include "imwidget/imwidget.h"
#include "midi/midi.h"
#include "proto/fti.pb.h"

namespace protones {

class NES;

class MidiSetup : public ImWindowBase {
  public:
    MidiSetup(NES* nes)
      : ImWindowBase(false, false),
      nes_(nes),
      enabled_(false),
      ports_(0),
      current_port_(0)
    {}
    bool Draw() override;
    void DrawEnvelope(proto::Envelope* envelope, proto::Envelope_Kind kind, InstrumentPlayer *player);

  private:
    void GetPortNames();

    NES* nes_;
    bool enabled_;
    int ports_;
    int current_port_;
    std::vector<std::string> portnames_;
    std::string current_channel_;
    std::string current_instrument_;
    const char* names_[128];
};

}  // namespace
#endif // PROTONES_IMWIDGET_MIDI_SETUP_H
