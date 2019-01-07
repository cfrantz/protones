#include <cstdio>
#include <cstdint>
#include <inttypes.h>
#include <memory>
#include <thread>

#include <gflags/gflags.h>
#include <gflags/gflags_declare.h>

#include "absl/memory/memory.h"
#include "app.h"
#include "imgui.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "imwidget/apu_debug.h"
#include "imwidget/controller_debug.h"
#include "imwidget/mem_debug.h"
#include "imwidget/ppu_debug.h"
#include "imwidget/error_dialog.h"
#include "nes/apu.h"
#include "nes/cartridge.h"
#include "nes/controller.h"
#include "nes/ppu.h"
#include "nes/nes.h"
#include "proto/config.pb.h"
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "util/browser.h"
#include "util/config.h"
#include "util/file.h"
#include "util/os.h"
#include "util/logging.h"
#include "util/imgui_impl_sdl.h"

#include "version.h"

#ifdef HAVE_NFD
#include "nfd.h"
#endif

DEFINE_bool(focus, false, "Whether joystick events require window focus");
DECLARE_double(volume);

namespace protones {
namespace py = pybind11;
using proto::ControllerButtons;

void ProtoNES::Init() {
    loaded_ = false;
    nes_ = absl::make_unique<NES>();
    scale_ = 4.0f;
    aspect_ = 1.2f;
    memset(frametime_, 0, sizeof(frametime_));
    ftp_ = 0;
    volume_ = FLAGS_volume;
    preferences_ = false;
    pause_ = false;
    step_ = false;

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
                FLAGS_focus ? "0" : "1");
    SDL_GL_SetSwapInterval(0);

    apu_debug_ = new APUDebug(nes_->apu());
    AddDrawCallback(apu_debug_);
    controller_debug_ = new ControllerDebug(nes_.get());
    AddDrawCallback(controller_debug_);
    mem_debug_ = new MemDebug(nes_->mem());
    AddDrawCallback(mem_debug_);
    ppu_tile_debug_ = new PPUTileDebug(nes_.get());
    AddDrawCallback(ppu_tile_debug_);
    ppu_vram_debug_ = new PPUVramDebug(nes_.get(), ppu_tile_debug_);
    AddDrawCallback(ppu_vram_debug_);


    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &nesimg_);
    glBindTexture(GL_TEXTURE_2D, nesimg_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nes_->ppu()->picture());
    InitControllers();
    InitAudio(44100, 1, APU::BUFFERLEN, AUDIO_F32);

    py::exec(R"py(
        from content.protones import *
    )py");

    console_ = absl::make_unique<PythonConsole>(
        hook_.attr("GetPythonConsole")());

    const auto& config = ConfigLoader<proto::Configuration>::GetConfig();
    for(const auto& b : config.controls().buttons()) {
        buttons_[b.scancode()] = b.button();
    }
    save_state_slot_ = 1;
    history_ptr_ = 0;
    history_enabled_ = false;
}

void ProtoNES::Import(const std::string& name) {
    py::module::import(name.c_str());
}

void ProtoNES::ProcessEvent(SDL_Event* event) {
    ImGuiIO& io = ImGui::GetIO();
    switch(event->type) {
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERAXISMOTION:
        nes_->controller(0)->set_buttons(event);
        break;
    case SDL_KEYDOWN:
        if (!io.WantCaptureKeyboard) {
            nes_->HandleKeyboard(event);
            ControllerButtons b = buttons_[event->key.keysym.scancode];
            switch(b) {
            case ControllerButtons::SaveSlot0:
            case ControllerButtons::SaveSlot1:
            case ControllerButtons::SaveSlot2:
            case ControllerButtons::SaveSlot3:
            case ControllerButtons::SaveSlot4:
            case ControllerButtons::SaveSlot5:
            case ControllerButtons::SaveSlot6:
            case ControllerButtons::SaveSlot7:
            case ControllerButtons::SaveSlot8:
            case ControllerButtons::SaveSlot9:
                save_state_slot_ = int(b - ControllerButtons::SaveSlot0);
                console_->AddLog("Save state slot set to %d", save_state_slot_);
                break;
            case ControllerButtons::StateReverse: {
                size_t i = (history_ptr_ - 1) % HISTORY_SIZE;
                if (nes_->LoadState(history_[i])) {
                    history_ptr_ = i;
                }
                break;
            }
            case ControllerButtons::StateForward: {
                size_t i = (history_ptr_ + 1) % HISTORY_SIZE;
                if (nes_->LoadState(history_[i])) {
                    history_ptr_ = i;
                }
                break;
            }

            default: ;
            }
        }
        break;
    case SDL_KEYUP:
        if (!io.WantCaptureKeyboard) {
            nes_->HandleKeyboard(event);
            ControllerButtons b = buttons_[event->key.keysym.scancode];
            switch(b) {
            case ControllerButtons::ControllerSaveState:
                SaveSlot(save_state_slot_);
                console_->AddLog("Save slot %d", save_state_slot_);
                break;
            case ControllerButtons::ControllerLoadState:
                LoadSlot(save_state_slot_);
                console_->AddLog("Load slot %d", save_state_slot_);
                break;
            default: ;
            }
        }
        break;

    default:
        ;
    }
}

void ProtoNES::SaveSlot(int slot) {
    std::string fn = os::path::DataPath({
            absl::StrCat(File::Basename(nes_->cartridge()->filename()),
                         ".state", slot)});
    nes_->SaveStateToFile(fn);
}

void ProtoNES::LoadSlot(int slot) {
    std::string fn = os::path::DataPath({
            absl::StrCat(File::Basename(nes_->cartridge()->filename()),
                         ".state", slot)});
    nes_->LoadStateFromFile(fn);
}

void ProtoNES::ProcessMessage(const std::string& msg, const void* extra) {
}

void ProtoNES::AudioCallback(void* stream, int len) {
    if (nes_) {
        nes_->apu()->PlayBuffer(stream, len);
    }
}

void ProtoNES::set_volume(float v) {
    volume_ = v;
    nes_->apu()->set_volume(volume_);
}

bool ProtoNES::PreDraw() {
    ImGuiIO& io = ImGui::GetIO();
    float width = io.DisplaySize.x * io.DisplayFramebufferScale.x;
    float height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glClearColor(clear_color_.x, clear_color_.y, clear_color_.z, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, nesimg_);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                  0, 0, 256, 240,
                  GL_RGBA, GL_UNSIGNED_BYTE, nes_->ppu()->picture());
    return true;
}

void ProtoNES::DrawPreferences() {
    if (!preferences_)
        return;

    ImGui::Begin("Preferences", &preferences_);
    float fta = 0;
    for(int i=0; i<100; i++) {
        fta += frametime_[i];
    }
    fta = 1.0f / (fta / 100.0f);
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Fps: %.1f, %.1f", io.Framerate, fta);

    ImGui::PushItemWidth(200);
    ImGui::DragFloat("Zoom", &scale_, 0.01f, 0.0f, 10.0f, "%.02f");
    ImGui::DragFloat("Aspect Ratio", &aspect_, 0.001f, 0.0f, 2.0f, "%.03f");
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f)) {
        nes_->apu()->set_volume(volume_);
    }
    ImGui::ColorEdit3("Clear Color", (float*)&clear_color_);
    ImGui::PopItemWidth();
    ImGui::End();
}

void ProtoNES::Draw() {
    ImGui::SetNextWindowSize(ImVec2(500,300), ImGuiSetCond_FirstUseEver);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
load_file:
                char *filename = nullptr;
                auto result = NFD_OpenDialog("nes", nullptr, &filename);
                if (result == NFD_OKAY) {
                    Load(filename);
                    save_filename_.assign(filename);
                }
                free(filename);
            }

            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (save_filename_.empty())
                    goto save_as;
                // DOSTUFF
            }
            if (ImGui::MenuItem("Save As")) {
save_as:
                char *filename = nullptr;
                auto result = NFD_SaveDialog("nes", nullptr, &filename);
                if (result == NFD_OKAY) {
                    std::string savefile = filename;
                    if (absl::EndsWith(savefile, ".nes")) {
                        save_filename_.assign(savefile);
                        // DOSTUFF
                    } else {
                        ErrorDialog::Spawn("Bad File Extension",
                            ErrorDialog::OK | ErrorDialog::CANCEL,
                            "Project files should have the extension .nes\n"
                            "If you want to save a .nes file, use File | Export\n\n"
                            "Press 'OK' to save anyway.\n")->set_result_cb(
                                [=](int result) {
                                    if (result == ErrorDialog::OK) {
                                        save_filename_.assign(savefile);
                                        // DOSTUFF
                                    }
                                });
                    }
                }
                free(filename);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Load State")) {
                LoadSlot(save_state_slot_);
            }
            if (ImGui::MenuItem("Save State")) {
                SaveSlot(save_state_slot_);
            }
            ImGui::Text("Save Slot"); ImGui::SameLine();
            ImGui::PushItemWidth(64);
            ImGui::Combo("##saveslot", &save_state_slot_,
                         "0\0001\0002\0003\0004\0005\0006\0007\0008\0009\000\000\000");
            ImGui::PopItemWidth();
            hook_.attr("FileMenu")();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) {
                running_ = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Debug Console", nullptr, &console_->visible());
            ImGui::MenuItem("Preferences", nullptr, &preferences_);
            ImGui::MenuItem("State History", nullptr, &history_enabled_);
            hook_.attr("EditMenu")();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Audio", nullptr, &apu_debug_->visible());
            ImGui::MenuItem("Controllers", nullptr, &controller_debug_->visible());
            ImGui::MenuItem("Memory", nullptr, &mem_debug_->visible());
            ImGui::MenuItem("PPU Tile Data", nullptr, &ppu_tile_debug_->visible());
            ImGui::MenuItem("PPU VRAM", nullptr, &ppu_vram_debug_->visible());
            hook_.attr("ViewMenu")();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Online Help")) {
                Help("root");
            }
            if (ImGui::MenuItem("About")) {
                SDL_version compiled, linked;
                SDL_VERSION(&compiled);
                SDL_GetVersion(&linked);
                ErrorDialog::Spawn("About ProtoNES",
                    "ProtoNES Emulator\n\n",
#ifdef BUILD_GIT_VERSION
                    "Version: ", BUILD_GIT_VERSION, "-", BUILD_SCM_STATUS, "\n"
#else
                    "Version: Unknown\n"
#warning "Built without version stamp"
#endif
                    "\nBuilt with SDL version ", compiled.major, ".",
                        compiled.minor, ".", compiled.patch, "\n"
                    "\nLinked with SDL version ", linked.major, ".",
                        linked.minor, ".", linked.patch, "\n"
                    );

            }
            hook_.attr("HelpMenu")();
            ImGui::EndMenu();
        }

        hook_.attr("MenuBar")();
        ImGui::EndMainMenuBar();
    }

    if (!loaded_) {
        goto load_file;
    }
    DrawPreferences();
    console_->Draw();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 20.0f * io.DisplayFramebufferScale.y));
    ImVec2 imgsz(256.0f * scale_ * aspect_, 240.0f *scale_);
    ImGui::SetNextWindowSize(imgsz);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("nesimg", nullptr, ImVec2(0, 0), 0.0f,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<ImTextureID>(nesimg_), ImVec2(0, 0), imgsz);
    hook_.attr("DrawImage")();
    ImGui::End();
    ImGui::PopStyleVar(3);

    hook_.attr("Draw")();
}

void ProtoNES::Run() {
    running_ = true;
    if (loaded_)
        nes_->Reset();

    while(running_) {
        py::gil_scoped_acquire gil;
        uint64_t f0 = nes_->frame();
        hook_.attr("EmulateFrame")();
        uint64_t f1 = nes_->frame();
        if (history_enabled_ && f0 != f1) {
            history_ptr_ = (history_ptr_ + 1) % HISTORY_SIZE;
            history_[history_ptr_] = nes_->SaveState();
        }
        BaseDraw();
        if (!ProcessEvents()) {
            running_ = false;
            break;
        }
    }
}

void ProtoNES::Load(const std::string& filename) {
    loaded_ = true;
    nes_->LoadFile(filename);
}

void ProtoNES::Help(const std::string& topickey) {
}

std::function<std::shared_ptr<ProtoNES>()> app_root;
void ProtoNES::set_python_root(std::shared_ptr<ProtoNES>& root) {
    app_root = [&](){ return root; };
}

PYBIND11_EMBEDDED_MODULE(app, m) {
    py::class_<ProtoNES, std::shared_ptr<ProtoNES>>(m, "ProtoNES")
        .def_property("clear_color", &ProtoNES::clear_color, &ProtoNES::set_clear_color)
        .def_property_readonly("nes", &ProtoNES::nes)
        .def_property("scale", &ProtoNES::scale, &ProtoNES::set_scale)
        .def_property("aspect", &ProtoNES::aspect, &ProtoNES::set_aspect)
        .def_property("volume", &ProtoNES::volume, &ProtoNES::set_volume)
        .def_property("hook", &ProtoNES::hook, &ProtoNES::set_hook);

    m.def("root", app_root);
}


}  // namespace protones
