#ifndef PROJECT_IMAPP_H
#define PROJECT_IMAPP_H
#include <memory>
#include <vector>

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"

#include "util/fpsmgr.h"
#include "imwidget/imwidget.h"

class ImApp {
  public:
    static ImApp* Get() { return singleton_; }
    ImApp(const std::string& name, int width, int height);
    ImApp(const std::string& name) : ImApp(name, 1280, 720) {}
    virtual ~ImApp();

    void InitControllers();
    void InitAudio(int freq, int chan, int bufsz, SDL_AudioFormat fmt);
    virtual void Init() {}
    virtual bool PreDraw() { return false; }
    virtual void Draw() {}
    virtual void ProcessEvent(SDL_Event* event) {}
    virtual void Help(const std::string& topickey) {}

    void SetTitle(const std::string& title, bool with_appname=true);
    void Run();
    void BaseDraw();
    virtual bool ProcessEvents();

    virtual void ProcessMessage(const std::string& msg, const void *extra) {}
    void ProcessMessage(const std::string& msg) {
        ProcessMessage(msg, nullptr);
    }

    void AddDrawCallback(ImWindowBase* window);
    void HelpButton(const std::string& topickey, bool right_justify=false);

    bool running_;
    ImVec4 clear_color_;
  protected:
    virtual void AudioCallback(float* stream, int len);
    std::string name_;
    int width_;
    int height_;
    std::vector<std::unique_ptr<ImWindowBase>> draw_callback_;

  private:
    static void AudioCallback_(void* userdata, uint8_t* stream, int len);

    static ImApp* singleton_;

    SDL_Window *window_;
    SDL_Renderer *renderer_;
    SDL_Texture *texture_;
    SDL_PixelFormat *format_;
    SDL_GLContext glcontext_;
    FPSManager fpsmgr_;

    std::vector<std::unique_ptr<ImWindowBase>> draw_added_;
};

#endif // PROJECT_IMAPP_H
