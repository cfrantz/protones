#include <algorithm>

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


proto::Envelope* EnvelopePointer(proto::FTInstrument *instrument, proto::Envelope_Kind kind) {
    switch(kind) {
        case proto::Envelope_Kind_VOLUME: return instrument->mutable_volume();
        case proto::Envelope_Kind_ARPEGGIO: return instrument->mutable_arpeggio();
        case proto::Envelope_Kind_PITCH: return instrument->mutable_pitch();
        case proto::Envelope_Kind_HIPITCH: return instrument->mutable_hipitch();
        case proto::Envelope_Kind_DUTY: return instrument->mutable_duty();
        default:
            return nullptr;
    }
}

struct EnvRange {
    int min, max;
};

EnvRange EnvelopeRange(proto::Envelope_Kind kind) {
    switch(kind) {
        case proto::Envelope_Kind_VOLUME: return EnvRange { 0, 15 };
        case proto::Envelope_Kind_ARPEGGIO: return EnvRange { -128, 127 };
        case proto::Envelope_Kind_PITCH: return EnvRange { -128, 127 };
        case proto::Envelope_Kind_HIPITCH: return EnvRange { -128, 127 };
        case proto::Envelope_Kind_DUTY: return EnvRange {0, 3 };
        default:
            return EnvRange {-128, 127};
    }
}

void MidiSetup::DrawEnvelope(proto::Envelope* envelope, proto::Envelope_Kind kind, InstrumentPlayer* player) {
    bool enabled = envelope->kind() == kind;
    if (ImGui::Checkbox("Enabled", &enabled)) {
        envelope->set_kind(enabled ? kind : proto::Envelope_Kind_UNKNOWN);
    }

    ImGui::Text("Envelope:       ");
    EnvRange range = EnvelopeRange(kind);
    int n = -1;
    auto* player_env = player ? player->envelope(kind) : nullptr;
    int frame = player_env ? player_env->frame() : -1;
    for(auto& val : *envelope->mutable_sequence()) {
        ImGui::SameLine();
        ImGui::PushID(++n);
        ImGui::BeginGroup();
        if (n == frame) ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFFFFFFFF);
        ImGui::VSliderInt("##envseq", ImVec2(20, 256), &val, range.min, range.max);
        if (n == frame) ImGui::PopStyleColor(1);
        int loopsel = n == envelope->loop() ? 1 :
                      n == envelope->release() ? 2 : 0;
        const char *loopitems[] = {" ", "Loop", "Release" };
        ImGui::PushItemWidth(20);

        if (ImGui::BeginCombo("##loop", loopitems[loopsel], ImGuiComboFlags_NoArrowButton)) {
            for(int i=0; i<3; i++) {
                bool selected = loopsel == i;
                if (ImGui::Selectable(loopitems[i], selected)) {
                    loopsel = i;
                    switch(loopsel) {
                        case 0:
                            if (n == envelope->loop()) envelope->set_loop(-1);
                            if (n == envelope->release()) envelope->set_release(-1);
                            break;
                        case 1:
                            envelope->set_loop(n);
                            if (n == envelope->release()) envelope->set_release(-1);
                            break;
                        case 2:
                            envelope->set_release(n);
                            if (n == envelope->loop()) envelope->set_loop(-1);
                            break;
                    }
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();
        ImGui::PopID();
    }

    int steps = envelope->sequence_size();
    ImGui::PushItemWidth(128);
    ImGui::Text("Envelope Steps: ");
    ImGui::SameLine();
    if (ImGui::InputInt("#envsteps", &steps, 1, 1)) {
        steps = std::clamp(steps, 0, 256);
        envelope->mutable_sequence()->Resize(steps, 0);
    }
    ImGui::PopItemWidth();

    char text[4096] = {0};
    size_t p = 0;
    for(int i=0; i<envelope->sequence_size(); i++) {
        if (i == envelope->release()) p += sprintf(text+p, "]");
        if (i > 0) p += sprintf(text+p, " ");
        if (i == envelope->loop()) p += sprintf(text+p, "[");
        p+= sprintf(text+p, "%d", envelope->sequence(i));
    }
    ImGui::Text("Envelope Values:");
    ImGui::SameLine();
    if (ImGui::InputText("##envtext", text, sizeof(text), ImGuiInputTextFlags_EnterReturnsTrue)) {
        char *p = text;
        envelope->mutable_sequence()->Clear();
        while(*p) {
            if (*p == ' ') { p++; continue; }
            if (*p == '[') {
                envelope->set_loop(envelope->sequence_size());
                p++; continue;
            }
            if (*p == ']') {
                envelope->set_release(envelope->sequence_size());
                p++; continue;
            }
            if (isdigit(*p)) {
                int val = strtol(p, &p, 0);
                envelope->add_sequence(val);
            } else {
                fprintf(stderr, "Unknown character in envelope sequence: '%c'\n", *p);
                p++;
            }
        }
    }
}

bool MidiSetup::Draw() {
    if (!visible_)
        return false;

    GetPortNames();
    ImGui::Begin("Midi Setup", &visible_);
    auto* midi = nes_->midi();
    RtMidiIn* port = nes_->midi()->midi();
    if (ImGui::Checkbox("Enabled", &enabled_)) {
        if (enabled_) {
            port->openPort(current_port_);
        } else {
            port->closePort();
        }
        midi->set_enabled(enabled_);
        current_channel_ = midi->config_.channel(0).name();
        current_instrument_ = midi->config_.channel(0).instrument();
    }
    ImGui::PushItemWidth(400);
    if (ImGui::Combo("Midi Port", &current_port_, names_, ports_)) {
            port->closePort();
            port->openPort(current_port_);
    }
    ImGui::PopItemWidth();


    if (!midi->channel_.empty()) {
        ImGui::Separator();
        ImGui::PushItemWidth(400);
        if (ImGui::BeginCombo("Channel", current_channel_.c_str())) {
            for(const auto& c : midi->config_.channel()) {
                bool selected = current_channel_ == c.name();
                if (ImGui::Selectable(c.name().c_str(), selected)) {
                    current_channel_ = c.name();
                    current_instrument_ = c.instrument();
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Instrument", current_instrument_.c_str())) {
            for(const auto& inst : midi->config_.instruments()) {
                bool selected = current_instrument_ == inst.first;
                if (ImGui::Selectable(inst.first.c_str(), selected)) {
                    current_instrument_ = inst.first;
                    midi->channel_[current_channel_]->set_instrument(current_instrument_);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if (!current_channel_.empty()) {
            const char *names[] = {"", "Volume", "Arpeggio", "Pitch", "HiPitch", "Duty" };
            auto* instrument = midi->channel_[current_channel_]->instrument_;
            auto* player = midi->channel_[current_channel_]->now_playing(instrument);
            if (ImGui::BeginTabBar("Envelopes", ImGuiTabBarFlags_None)) {
                for(int i=1; i<6; i++) {
                    auto kind = static_cast<proto::Envelope_Kind>(i);
                    if (ImGui::BeginTabItem(names[i])) {
                        auto* env = EnvelopePointer(instrument, kind);
                        DrawEnvelope(env, kind, player);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
    }

    ImGui::End();
    return false;
}

}  // namespace protones
