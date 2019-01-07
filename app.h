#ifndef PROTONES_APP_H
#define PROTONES_APP_H
#include <map>
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "pybind11/embed.h"
#include "imwidget/imapp.h"
#include "imwidget/python_console.h"
#include "nes/nes.h"
#include "proto/controller.pb.h"

namespace protones {

class APUDebug;
class ControllerDebug;
class MemDebug;
class PPUTileDebug;
class PPUVramDebug;


class ProtoNES: public ImApp {
  public:
    ProtoNES(const std::string& name) : ImApp(name, 1280, 720) {}
    ~ProtoNES() override {}

    void Load(const std::string& filename);

    void Init() override;
    void ProcessEvent(SDL_Event* event) override;
    void ProcessMessage(const std::string& msg, const void* extra) override;
    bool PreDraw() override;
    void Draw() override;
    void Run();
    void DrawPreferences();

    void AudioCallback(void* stream, int len) override;
    void Help(const std::string& topickey);

    std::shared_ptr<NES> nes() { return nes_; }
    float scale() { return scale_; }
    float aspect() { return aspect_; }
    float volume() { return volume_; }
    void set_scale(float s) { scale_ = s; }
    void set_aspect(float a) { aspect_ = a; }
    void set_volume(float v);
    static void set_python_root(std::shared_ptr<ProtoNES>& root);

    pybind11::object& hook() { return hook_; }
    void set_hook(const pybind11::object& h) { hook_ = h; }
    void Import(const std::string& name);

    void SaveSlot(int slot);
    void LoadSlot(int slot);

    const static size_t HISTORY_SIZE = 256;
  private:
    bool loaded_;
    bool pause_;
    bool step_;
    GLuint nesimg_;
    float scale_;
    float aspect_;
    float volume_;
    bool preferences_;
    int save_state_slot_;
    std::shared_ptr<NES> nes_;
    std::string save_filename_;

    std::string history_[HISTORY_SIZE];
    size_t history_ptr_;
    bool history_enabled_;

    APUDebug* apu_debug_;
    ControllerDebug* controller_debug_;
    MemDebug* mem_debug_;
    PPUTileDebug* ppu_tile_debug_;
    PPUVramDebug* ppu_vram_debug_;
    std::unique_ptr<PythonConsole> console_;
    std::map<int, proto::ControllerButtons> buttons_;

    float frametime_[100];
    int ftp_;
    pybind11::object hook_;
};

}  // namespace protones
#endif // PROTONES_APP_H
