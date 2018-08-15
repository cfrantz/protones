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
#include "imwidget/apu_debug.h"
#include "imwidget/controller_debug.h"
#include "imwidget/mem_debug.h"
#include "imwidget/ppu_debug.h"
#include "imwidget/error_dialog.h"
#include "proto/config.pb.h"
#include "nes/apu.h"
#include "nes/cartridge.h"
#include "nes/controller.h"
#include "nes/ppu.h"
#include "nes/nes.h"
#include "util/browser.h"
#include "util/config.h"
#include "util/os.h"
#include "util/logging.h"
#include "util/imgui_impl_sdl.h"

#include "version.h"

#ifdef HAVE_NFD
#include "nfd.h"
#endif

DECLARE_double(volume);

namespace protones {

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

    const auto& config = ConfigLoader<proto::Configuration>::GetConfig();
    for(const auto& b : config.controls().buttons()) {
        buttons_[b.scancode()] = b.button();
    }

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
}

void ProtoNES::ProcessEvent(SDL_Event* event) {
    switch(event->type) {
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERAXISMOTION:
        nes_->controller(0)->set_buttons(event);
        break;
    case SDL_KEYDOWN: {
        ControllerButtons b = buttons_[event->key.keysym.scancode];
        switch (b) {
        case ControllerButtons::ControllerPause:
            pause_ = !pause_;
            break;
        case ControllerButtons::ControllerFrameStep:
            pause_ = true;
            step_ = true;
            break;
        case ControllerButtons::ControllerReset:
            nes_->Reset();
            break;
        default:
            nes_->controller(0)->set_buttons(event);
        }
        }
        break;
    case SDL_KEYUP:
        nes_->controller(0)->set_buttons(event);
        break;
    default:
        ;
    }
}

void ProtoNES::ProcessMessage(const std::string& msg, const void* extra) {
}

void ProtoNES::AudioCallback(void* stream, int len) {
    if (nes_) {
        nes_->apu()->PlayBuffer(stream, len);
    }
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

    glBegin(GL_QUADS);
    float x0 = 0.0, y0 = 20.0 * io.DisplayFramebufferScale.y;
    glTexCoord2f(0, 0); glVertex2f(x0, y0);
    glTexCoord2f(1, 0); glVertex2f(x0 + 256 * scale_ * aspect_, y0);
    glTexCoord2f(1, 1); glVertex2f(x0 + 256 * scale_ * aspect_, y0 + 240 * scale_);
    glTexCoord2f(0, 1); glVertex2f(x0, y0 + 240 * scale_);
    glEnd();
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
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Debug Console", nullptr, &console_.visible());
            ImGui::MenuItem("Preferences", nullptr, &preferences_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Audio", nullptr, &apu_debug_->visible());
            ImGui::MenuItem("Controllers", nullptr, &controller_debug_->visible());
            ImGui::MenuItem("Memory", nullptr, &mem_debug_->visible());
            ImGui::MenuItem("PPU Tile Data", nullptr, &ppu_tile_debug_->visible());
            ImGui::MenuItem("PPU VRAM", nullptr, &ppu_vram_debug_->visible());
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
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (!loaded_) {
        goto load_file;
    }
    DrawPreferences();
}

void ProtoNES::Run() {
    running_ = true;
    if (loaded_)
        nes_->Reset();
    std::thread emulator(&ProtoNES::EmulateInThread, this);
    while(running_) {
        BaseDraw();
        if (!ProcessEvents()) {
            running_ = false;
            break;
        }
    }
    emulator.join();
}

void ProtoNES::EmulateInThread() {
    while(running_) {
        if (!loaded_) {
            continue;
        }
        if (pause_) {
            os::SchedulerYield();
            if (!step_) {
                continue;
            }
            step_ = false;
        }
        int64_t t0 = os::utime_now();
        nes_->EmulateFrame();
        int64_t t1 = os::utime_now();
        frametime_[ftp_] = (t1-t0) / 1e6;
        ftp_ = (ftp_ + 1) % 100;
    }
}

void ProtoNES::Load(const std::string& filename) {
    loaded_ = true;
    nes_->LoadFile(filename);
}

void ProtoNES::Help(const std::string& topickey) {
}

}  // namespace protones
