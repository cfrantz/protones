#ifndef PROTONES_NES_CONTROLLER_H
#define PROTONES_NES_CONTROLLER_H
#include <cstdint>
#include <vector>
#include <SDL2/SDL.h>
#include "nes/base.h"
#include "nes/nes.h"
namespace protones {

class Controller : public EmulatedDevice {
  public:
    Controller(NES* nes, int cnum);
    ~Controller() {}
    uint8_t Read();
    void Write(uint8_t val);
    inline uint8_t buttons() { return buttons_; }
    inline void set_buttons(int b) { buttons_ = uint8_t(b); }
    void set_buttons(SDL_Event* event);
    void AppendButtons(uint8_t b);
    void Emulate();
    void DebugStuff();

    static const int BUTTON_A      = 0x01;
    static const int BUTTON_B      = 0x02;
    static const int BUTTON_SELECT = 0x04;
    static const int BUTTON_START  = 0x08;
    static const int BUTTON_UP     = 0x10;
    static const int BUTTON_DOWN   = 0x20;
    static const int BUTTON_LEFT   = 0x40;
    static const int BUTTON_RIGHT  = 0x80;
  private:
    NES* nes_;
    uint8_t buttons_;
    int index_, strobe_;
    std::vector<uint8_t> movie_;
    bool got_read_;
    int cnum_;
};

}  // namespace protones
#endif // PROTONES_NES_CONTROLLER_H
