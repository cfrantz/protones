#include "imwidget/apu_debug.h"

#include "imgui.h"
#include "nes/apu.h"

namespace protones {

void APUDebug::DrawPulse(Pulse* pulse) {
    ImGui::BeginGroup();
    ImGui::PlotLines("", pulse->dbgbuf_, pulse->DBGBUFSZ, pulse->dbgp_,
                     "Pulse", 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", pulse->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", pulse->reg_.control);
    ImGui::Text("Sweep:   %02X", pulse->reg_.sweep);
    ImGui::Text("Timer:   %02X%02X", pulse->reg_.thi, pulse->reg_.tlo);
    ImGui::EndGroup();
    ImGui::EndGroup();
}

void APUDebug::DrawTriangle(Triangle* tri) {
    ImGui::BeginGroup();
    ImGui::PlotLines("", tri->dbgbuf_, tri->DBGBUFSZ, tri->dbgp_,
                     "Triangle", 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", tri->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", tri->reg_.control);
    ImGui::Text("Timer:   %02X%02X", tri->reg_.thi, tri->reg_.tlo);
    ImGui::EndGroup();
    ImGui::EndGroup();
}

void APUDebug::DrawNoise(Noise* noise) {
    ImGui::BeginGroup();
    ImGui::PlotLines("", noise->dbgbuf_, noise->DBGBUFSZ, noise->dbgp_,
                     "Noise", 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", noise->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", noise->reg_.control);
    ImGui::Text("Period: %02X", noise->reg_.period);
    ImGui::Text("Length: %02X", noise->reg_.length);
    ImGui::EndGroup();
    ImGui::EndGroup();
}

void APUDebug::DrawDMC(DMC* dmc) {
    ImGui::BeginGroup();
    ImGui::PlotLines("", dmc->dbgbuf_, dmc->DBGBUFSZ, dmc->dbgp_,
                     "DMC", 0.0f, 15.0f, ImVec2(0, 80));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Enabled: %s", dmc->enabled_ ? "true" : "false");
    ImGui::Text("Control: %02X", dmc->reg_.control);
    ImGui::Text("Value:   %02X", dmc->reg_.value);
    ImGui::Text("Address: %02X", dmc->reg_.address);
    ImGui::Text("Length:  %02X", dmc->reg_.length);
    ImGui::EndGroup();
    ImGui::EndGroup();
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
    ImGui::End();
    return false;
}

}  // namespace protones
