#include <cmath>
#include "imwidget/apu_debug.h"

#include "imgui.h"
#include "nes/apu.h"
#include "nes/mapper.h"

namespace protones {

constexpr double TRT = std::pow(2.0, 1.0/12.0);
constexpr double CPU = 1789773;
namespace {
inline double frequency(double t) {
    return CPU / (16.0 * (t + 1));
}
}

const char* APUDebug::notes_[128] = {
                          "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1",
    "A-1", "A#-1", "B-1", "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0",
    "A0", "A#0", "B0", "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1",
    "A1", "A#1", "B1", "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2",
    "A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3",
    "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4",
    "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5",
    "A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6",
    "A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7",
    "A7", "A#7", "B7", "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8",
    "A8", "A#8", "B8", "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9",
};


void APUDebug::InitFreqTable(double a4) {
    // am1 is the frequency of midi note #9 AKA "A-1"
    double am1 = a4 / 32.0;
    for(int i=0; i<9; ++i) {
        freq_[i] = 0;
    }
    for(int i=9; i<128; ++i) {
        freq_[i] = am1 * std::pow(TRT, double(i-9));
    }
}

int APUDebug::FindFreqIndex(double f) {
    for(int i=0; i<127; ++i) {
        if (f >= freq_[i] && f < freq_[i+1]) {
            return i;
        }
    }
    return 0;
}

void APUDebug::DrawPulse(Pulse* pulse) {
    ImGui::PushID(pulse);
    ImGui::BeginGroup();
    ImGui::PlotLines("", pulse->dbgbuf_, pulse->DBGBUFSZ, pulse->dbgp_,
                     pulse->name(), 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", pulse->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", pulse->reg_.control);
    ImGui::Text("Sweep:   %02X", pulse->reg_.sweep);
    ImGui::Text("Timer:   %02X%02X", pulse->reg_.thi, pulse->reg_.tlo);
    double f = frequency(pulse->timer_period_);
    ImGui::Text("Note:    %-3s %.2f", notes_[FindFreqIndex(f)], f);
    ImGui::EndGroup();
    ImGui::SliderFloat("Volume", pulse->mutable_output_volume(), 0.0f, 1.0f);
    ImGui::EndGroup();
    ImGui::PopID();
}

void APUDebug::DrawTriangle(Triangle* tri) {
    ImGui::PushID(tri);
    ImGui::BeginGroup();
    ImGui::PlotLines("", tri->dbgbuf_, tri->DBGBUFSZ, tri->dbgp_,
                     tri->name(), 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", tri->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", tri->reg_.control);
    ImGui::Text("Timer:   %02X%02X", tri->reg_.thi, tri->reg_.tlo);
    double f = frequency(tri->timer_period_) / 2.0;
    ImGui::Text("Note:    %-3s %.2f", notes_[FindFreqIndex(f)], f);
    ImGui::EndGroup();
    ImGui::SliderFloat("Volume", tri->mutable_output_volume(), 0.0f, 1.0f);
    ImGui::EndGroup();
    ImGui::PopID();
}

void APUDebug::DrawNoise(Noise* noise) {
    ImGui::PushID(noise);
    ImGui::BeginGroup();
    ImGui::PlotLines("", noise->dbgbuf_, noise->DBGBUFSZ, noise->dbgp_,
                     noise->name(), 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", noise->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", noise->reg_.control);
    ImGui::Text("Period: %02X", noise->reg_.period);
    ImGui::Text("Length: %02X", noise->reg_.length);
    ImGui::EndGroup();
    ImGui::SliderFloat("Volume", noise->mutable_output_volume(), 0.0f, 1.0f);
    ImGui::EndGroup();
    ImGui::PopID();
}

void APUDebug::DrawDMC(DMC* dmc) {
    ImGui::PushID(dmc);
    ImGui::BeginGroup();
    ImGui::PlotLines("", dmc->dbgbuf_, dmc->DBGBUFSZ, dmc->dbgp_,
                     dmc->name(), 0.0f, 127.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", dmc->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", dmc->reg_.control);
    ImGui::Text("Value:   %02X", dmc->reg_.value);
    ImGui::Text("Address: %02X ($%04x)", dmc->reg_.address,
            0xC000 | uint16_t(dmc->reg_.address) << 6);
    ImGui::Text("Length:  %02X ($%04x)", dmc->reg_.length,
            0x0001 | uint16_t(dmc->reg_.length) << 4);
    ImGui::EndGroup();
    ImGui::SliderFloat("Volume", dmc->mutable_output_volume(), 0.0f, 1.0f);
    ImGui::EndGroup();
    ImGui::PopID();
}

void APUDebug::DrawGeneric(APUDevice* dev) {
    ImGui::PushID(dev);
    ImGui::BeginGroup();
    ImGui::PlotLines("", dev->dbgbuf_, dev->DBGBUFSZ, dev->dbgp_,
                     dev->name(), -1.0f, 1.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    const char *t;
    size_t n = 0;
    while((t = dev->text(n)) != nullptr) {
        ImGui::Text("%s", t);
        n++;
    }
    ImGui::EndGroup();
    ImGui::SliderFloat("Volume", dev->mutable_output_volume(), 0.0f, 1.0f);
    ImGui::EndGroup();
    ImGui::PopID();
}

void APUDebug::DrawOne(APUDevice *dev) {
    switch(dev->type()) {
        case APUDevice::Type::Pulse:
            DrawPulse(static_cast<Pulse*>(dev));
            break;
        case APUDevice::Type::Triangle:
            DrawTriangle(static_cast<Triangle*>(dev));
            break;
        case APUDevice::Type::Noise:
            DrawNoise(static_cast<Noise*>(dev));
            break;
        case APUDevice::Type::DMC:
            DrawDMC(static_cast<DMC*>(dev));
            break;
        case APUDevice::Type::VRC7:
            DrawGeneric(dev);
            break;
        default:
            // unknown
            fprintf(stderr, "Unknown sound device %d\n", int(dev->type()));
    }
}

bool APUDebug::Draw() {
    if (!visible_)
        return false;

    ImGui::Begin("Audio", &visible_);
    DrawPulse(&apu_->pulse_[0]);    
    DrawPulse(&apu_->pulse_[1]);    
    DrawTriangle(&apu_->triangle_);
    DrawNoise(&apu_->noise_);
    DrawDMC(&apu_->dmc_);
    for(const auto& dev : apu_->nes_->mapper()->DebugExpansionAudio()) {
        DrawOne(dev);
    }

    ImGui::End();
    return false;
}

}  // namespace protones
