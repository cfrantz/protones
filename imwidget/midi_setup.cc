#include <algorithm>
#include <ctype.h>

#include "imwidget/midi_setup.h"

#include "imgui.h"
#include "nfd.h"
#include "nes/nes.h"
#include "midi/fti.h"
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
        if (n == frame) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF999999);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0xFFCCCCCC);
        }
        ImGui::VSliderInt("##envseq", ImVec2(20, 256), &val, range.min, range.max);
        if (n == frame) ImGui::PopStyleColor(2);
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
    if (ImGui::InputInt("##envsteps", &steps, 1, 1)) {
        steps = std::clamp(steps, 0, 256);
        envelope->mutable_sequence()->Resize(steps, 0);
    }
    ImGui::PopItemWidth();

    char text[4096] = {0};
    size_t p = 0;
    for(int i=0; i<envelope->sequence_size(); i++) {
        if (i > 0) p += sprintf(text+p, " ");
        if (i == envelope->loop()) p += sprintf(text+p, "[");
        p+= sprintf(text+p, "%d", envelope->sequence(i));
        if (i+1 == envelope->release()) p += sprintf(text+p, "]");
    }
    ImGui::Text("Envelope Values:");
    ImGui::SameLine();
    if (ImGui::InputText("##envtext", text, sizeof(text), ImGuiInputTextFlags_EnterReturnsTrue)) {
        char *p = text;
        envelope->mutable_sequence()->Clear();
        envelope->set_loop(-1);
        envelope->set_release(-1);
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
            if (isdigit(*p) || *p == '-' || *p == '+') {
                int val = strtol(p, &p, 0);
                envelope->add_sequence(val);
            } else {
                fprintf(stderr, "Unknown character in envelope sequence: '%c'\n", *p);
                p++;
            }
        }
    }
}

void MidiSetup::DrawVRC7(proto::FTInstrument* inst) {
    const char *instruments[] = {
        // Name in FamiTracker, Name in https://wiki.nesdev.org/w/index.php?title=VRC7_audio
        "Custom",
        "Bell",               // "Buzzy Bell",
        "Guitar",
        "Piano",              // "Wurly",
        "Flute",
        "Clarinet",
        "Rattling Bell",      // "Synth",
        "Trumpet",
        "Organ",
        "Soft Bell",          // "Bells",
        "Xylophone",          // "Vibes",
        "Vibraphone",
        "Brass",              // "Tutti",
        "Bass Guitar",        // "Fretless",
        "Synthesizer",        // "Synth Bass",
        "Chorus",             // "Sweep",
    };

    ImGui::PushItemWidth(400);
    uint32_t patch = inst->vrc7().patch();
    if (ImGui::BeginCombo("Patch", instruments[patch])) {
        for(uint32_t i=0; i<16; i++) {
            bool selected = i == patch;
            if (ImGui::Selectable(instruments[i], selected)) {
                inst->mutable_vrc7()->set_patch(i);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (inst->vrc7().patch() == 0) {
        uint8_t regs[8] = {0};
        size_t n = 0;
        for(const auto &r : inst->vrc7().regs()) {
            regs[n++] = r;
        }
        bool changed = false;
        VRC7Patch modulator = VRC7Patch::Modulator(regs);
        VRC7Patch carrier = VRC7Patch::Carrier(regs);
        ImGui::BeginGroup();
        ImGui::PushID("modulator");
        ImGui::PushItemWidth(400);
        ImGui::Text("Modulator Parameters");
        changed |= ImGui::Checkbox("Tremolo", &modulator.tremolo);
        changed |= ImGui::Checkbox("Vibrato", &modulator.vibrato);
        changed |= ImGui::Checkbox("Sustain", &modulator.sustain);
        changed |= ImGui::Checkbox("Key Rate Scaling", &modulator.key_rate);
        changed |= ImGui::Checkbox("Half Rectified", &modulator.waveform);
        changed |= ImGui::SliderInt("Multiplier", &modulator.multiplier, 0, 15);
        changed |= ImGui::SliderInt("Key Scaling", &modulator.key_scaling, 0, 3);
        changed |= ImGui::SliderInt4("ADSR", modulator.adsr, 0, 15);
        changed |= ImGui::SliderInt("Level", &modulator.output, 0, 63);
        changed |= ImGui::SliderInt("Feedback", &modulator.feedback, 0, 7);
        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::PushID("carrier");
        ImGui::PushItemWidth(400);
        ImGui::Text("Carrier Parameters");
        changed |= ImGui::Checkbox("Tremolo", &carrier.tremolo);
        changed |= ImGui::Checkbox("Vibrato", &carrier.vibrato);
        changed |= ImGui::Checkbox("Sustain", &carrier.sustain);
        changed |= ImGui::Checkbox("Key Rate Scaling", &carrier.key_rate);
        changed |= ImGui::Checkbox("Half Rectified", &carrier.waveform);
        changed |= ImGui::SliderInt("Multiplier", &carrier.multiplier, 0, 15);
        changed |= ImGui::SliderInt("Key Scaling", &carrier.key_scaling, 0, 3);
        changed |= ImGui::SliderInt4("ADSR", carrier.adsr, 0, 15);
        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::EndGroup();


        if (changed) {
            modulator.ToRegs(regs);
            carrier.ToRegs(regs);
            inst->mutable_vrc7()->clear_regs();
            for(const auto &r : regs) {
                inst->mutable_vrc7()->add_regs(r);
            }
        }

        ImGui::Separator();
        char text[32];
        snprintf(text, sizeof(text), "%02X %02X %02X %02X %02X %02X %02X %02X",
                regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
        ImGui::PushItemWidth(400);
        if (ImGui::InputText("Register Values", text, sizeof(text), ImGuiInputTextFlags_EnterReturnsTrue)) {
            char *p = text;
            inst->mutable_vrc7()->clear_regs();
            while(*p) {
                if (isspace(*p)) { p++; continue; }
                if (isxdigit(*p)) {
                    int val = strtol(p, &p, 16);
                    inst->mutable_vrc7()->add_regs(val);
                } else {
                    fprintf(stderr, "Unknown character in VRC7 regs: '%c'\n", *p);
                    p++;
                }
            }
        }
        ImGui::PopItemWidth();
    }
}

bool MidiSetup::Draw() {
    if (!visible_)
        return false;

    GetPortNames();
    ImGui::Begin("Midi Setup", &visible_);
    auto* midi = nes_->midi();

    // Since the instruments map has arbitrary iteration order, lets build
    // a vector of names in sorted order.
    if (instruments_.empty()) {
        for(const auto& inst : midi->config_.instruments()) {
            instruments_.push_back(&inst.first);
        }
        std::sort(instruments_.begin(), instruments_.end(),
            [](const std::string* a, const std::string* b) {
                return *a < *b;
            }
        );
    }

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
    ImGui::SameLine();
    if (ImGui::Button("MIDI Panic")) {
        midi->MidiPanic();
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
            for(const auto* inst : instruments_) {
                bool selected = current_instrument_ == *inst;
                if (ImGui::Selectable(inst->c_str(), selected)) {
                    current_instrument_ = *inst;
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
            ImGui::Separator();
            auto* instrument = midi->channel_[current_channel_]->instrument_;
            char name[1024];
            snprintf(name, sizeof(name), "%s", instrument->name().c_str());
            ImGui::PushItemWidth(400);
            if (ImGui::InputText("FTI Instrument Name##name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                instrument->set_name(name);
            }
            ImGui::PopItemWidth();

            if (instrument->kind() == proto::FTInstrument_Kind_VRC7) {
                DrawVRC7(instrument);
            } else{
                const char *names[] = {"", "Volume", "Arpeggio", "Pitch", "HiPitch", "Duty" };
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

            if (ImGui::Button("Save Instrument")) {
                char *filename = nullptr;
                auto result = NFD_SaveDialog("fti", nullptr, &filename);
                if (result == NFD_OKAY) {
                    auto status = SaveFTI(filename, *instrument);
                    if (!status.ok()) {
                        fprintf(stderr, "Error saving instrument: %s", status.ToString().c_str());
                    }
                }
            }
        }
    }
    ImGui::End();
    return false;
}

}  // namespace protones
