#ifndef PROTONES_APP_H
#define PROTONES_APP_H
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "imwidget/imapp.h"
#include "nes/nes.h"

namespace protones {

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

    void AudioCallback(void* stream, int len) override;

    void Help(const std::string& topickey);
  private:
    bool loaded_;
    GLuint nesimg_;
    float scale_;
    float aspect_;
    std::unique_ptr<NES> nes_;
    std::string save_filename_;
};

}  // namespace protones
#endif // PROTONES_APP_H
