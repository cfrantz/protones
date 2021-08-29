#ifndef PROTONES_IMWIDGET_MIDI_SETUP_H
#define PROTONES_IMWIDGET_MIDI_SETUP_H
#include <string>
#include <vector>

#include "imwidget/imwidget.h"

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
  private:
    void GetPortNames();

    NES* nes_;
    bool enabled_;
    int ports_;
    int current_port_;
    std::vector<std::string> portnames_;
    const char* names_[128];
};

}  // namespace
#endif // PROTONES_IMWIDGET_MIDI_SETUP_H
