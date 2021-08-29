#include "imwidget/midi_setup.h"

#include "imgui.h"
#include "nes/nes.h"
#include "midi/midi.h"
#include "RtMidi.h"

namespace protones {

void MidiSetup::GetPortNames() {
    RtMidiIn* midi = nes_->midi()->midi();
    int ports = midi->getPortCount();
    portnames_.clear();
    for(int i=0; i<ports; ++i) {
        portnames_.push_back(midi->getPortName(i));
        names_[i] = portnames_.at(i).c_str();
    }
    ports_ = ports;
}

bool MidiSetup::Draw() {
    if (!visible_)
        return false;

    RtMidiIn* midi = nes_->midi()->midi();
    GetPortNames();
    ImGui::Begin("Midi Setup", &visible_);
    if (ImGui::Checkbox("Enabled", &enabled_)) {
        if (enabled_) {
            midi->openPort(current_port_);
        } else {
            midi->closePort();
        }
        nes_->midi()->set_enabled(enabled_);
    }
    if (ImGui::Combo("Midi Port", &current_port_, names_, ports_)) {
            midi->closePort();
            midi->openPort(current_port_);
    }
    ImGui::End();
    return false;
}

}  // namespace protones
