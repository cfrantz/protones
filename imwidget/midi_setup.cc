#include "imwidget/midi_setup.h"

#include "imgui.h"
#include "nes/nes.h"
#include "nes/midi.h"
#include "RtMidi.h"

namespace protones {

void MidiSetup::GetPortNames() {
    RtMidiOut* midi = nes_->midi()->out();
    int ports = midi->getPortCount();
    portnames_.clear();
    for(int i=0; i<ports; ++i) {
        portnames_.push_back(midi->getPortName(i));
        names_[i] = portnames_.at(i).c_str();
    }
    ports_ = ports;
}

bool MidiSetup::Draw() {
    const char *chanlbl[] = {
        "Channel 1",
        "Channel 2",
        "Channel 3",
        "Channel 4",
    };
    if (!visible_)
        return false;

    RtMidiOut* midi = nes_->midi()->out();
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
    if (ImGui::DragFloat("A440", &A4_, 1.0f, 300.0f, 500.0f, "%.1f")) {
        nes_->midi()->InitFreqTable(A4_);
    }
    for(int i=0; i<4; ++i) {
        ImGui::SliderFloat(chanlbl[i], &nes_->midi()->chan_volume(i), 0, 2.0, "%.02f");
    }
    ImGui::End();
    return false;
}

}  // namespace protones
