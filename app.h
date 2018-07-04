#ifndef PROTONES_APP_H
#define PROTONES_APP_H
#include <map>
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "proto/controller.pb.h"
#include "imwidget/imapp.h"
#include "nes/nes.h"

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
    void EmulateInThread();
    void DrawPreferences();

    void AudioCallback(void* stream, int len) override;
    void Help(const std::string& topickey);
  private:
    bool loaded_;
    bool pause_;
    bool step_;
    GLuint nesimg_;
    float scale_;
    float aspect_;
    float volume_;
    bool preferences_;
    std::unique_ptr<NES> nes_;
    std::string save_filename_;

    APUDebug* apu_debug_;
    ControllerDebug* controller_debug_;
    MemDebug* mem_debug_;
    PPUTileDebug* ppu_tile_debug_;
    PPUVramDebug* ppu_vram_debug_;
    float frametime_[100];
    int ftp_;
    std::map<int, proto::ControllerButtons> buttons_;
};

}  // namespace protones
#endif // PROTONES_APP_H
