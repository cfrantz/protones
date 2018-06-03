#include "imwidget/controller_debug.h"

#include "imgui.h"
#include "nes/nes.h"
#include "nes/controller.h"

namespace protones {

void ControllerDebug::DrawController(Controller* c) {
    auto on = ImColor(0xFFFFFFFF);
    auto off = ImColor(0xFF808080);
    ImGui::BeginGroup();
    ImGui::Text("Controller %d", c->cnum_);
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_UP) ? on : off, " U");
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_LEFT) ? on : off, "L");
    ImGui::SameLine();
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_RIGHT) ? on : off, "R");
    ImGui::SameLine();
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_SELECT) ? on : off, "Sel");
    ImGui::SameLine();
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_START) ? on : off, "Sta");
    ImGui::SameLine();
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_B) ? on : off, " B");
    ImGui::SameLine();
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_A) ? on : off, "A");
    ImGui::TextColored((c->buttons_ & Controller::BUTTON_DOWN) ? on : off, " D");
    ImGui::EndGroup();
}

bool ControllerDebug::Draw() {
    if (!visible_)
        return false;

    ImGui::Begin("Controllers", &visible_);
    for(int i=0; i<2; i++) {
        if (i) ImGui::SameLine();
        DrawController(nes_->controller(i));
    }
    ImGui::End();
    return false;
}

}  // namespace protones
