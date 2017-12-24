#include <cstdio>
#include <cstdint>
#include <inttypes.h>
#include <memory>

#include <gflags/gflags.h>

#include "absl/memory/memory.h"
#include "app.h"
#include "imgui.h"
#include "absl/strings/match.h"
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
#include "util/browser.h"
#include "util/os.h"
#include "util/logging.h"
#include "util/imgui_impl_sdl.h"

#include "version.h"

#ifdef HAVE_NFD
#include "nfd.h"
#endif


namespace protones {

void ProtoNES::Init() {
    loaded_ = false;
    nes_ = absl::make_unique<NES>();
    scale_ = 4.0f;
    aspect_ = 1.2f;

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
            ImGui::MenuItem("Debug Console", nullptr,
                            &console_.visible());
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
                ErrorDialog::Spawn("About ProtoNES",
                    "ProtoNES Emulator\n\n",
#ifdef BUILD_GIT_VERSION
                    "Version: ", BUILD_GIT_VERSION, "-", BUILD_SCM_STATUS
#else
                    "Version: Unknown"
#warning "Built without version stamp"
#endif
                    );

            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    static bool open = true;
    if (ImGui::Begin("MyDebug", &open)) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Fps: %.1f", io.Framerate);
    }
    ImGui::End();

    if (!loaded_) {
        goto load_file;
    }
}

void ProtoNES::Run() {
    running_ = true;
    if (loaded_)
        nes_->Reset();
    while(running_) {
        BaseDraw();
        if (!ProcessEvents())
            break;
        if (loaded_) {
            nes_->EmulateFrame();
        }
    }

}

void ProtoNES::Load(const std::string& filename) {
    loaded_ = true;
    nes_->LoadFile(filename);
}

void ProtoNES::Help(const std::string& topickey) {
}

}  // namespace protones
