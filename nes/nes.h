#ifndef PROTONES_NES_NES_H
#define PROTONES_NES_NES_H
#include <string>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>

#include "nes/base.h"
#include "proto/nes.pb.h"
namespace protones {

class APU;
class Cpu;
class Cartridge;
class Controller;
class Debugger;
class Mapper;
class Mem;
class PPU;

class NES {
  public:
    NES();
    void LoadFile(const std::string& filename);
    void IRQ();
    void NMI();

    inline int controller_size() const {
        return int(sizeof(controller_) / sizeof(controller_[0]));
    }
    inline APU* apu() { return apu_; }
    inline Cpu* cpu() { return cpu_; }
    inline PPU* ppu() { return ppu_; }
    inline Mem* mem() { return mem_; }
    inline Mapper* mapper() { return mapper_; }
    inline Cartridge* cartridge() { return cart_; }
    inline Controller* controller(int n) { return controller_[n]; }
    inline uint32_t palette(uint8_t c) { return palette_[c % 64]; }
    inline uint64_t frame() { return frame_; }

    int cpu_cycles();
    inline void Stall(int s) { stall_ += s; }

    void Reset();
    bool Emulate();
    bool EmulateFrame();

    void LoadState(const std::string& filename);
    void SaveState(const std::string& filename, bool text=false);

    static const int frequency = 1789773;
    static constexpr double frame_counter_rate = frequency / 240.0;
    static constexpr double sample_rate = frequency / 44100.0;
  private:
    void DebugPalette(bool* active);
    void HandleKeyboard(SDL_Event* event);
    APU* apu_;
    Cpu* cpu_;
    PPU* ppu_;
    Mem* mem_;
    Mapper* mapper_;
    Cartridge* cart_;
    Controller* controller_[4];
    std::vector<std::unique_ptr<EmulatedDevice>> devices_;

    proto::NES state_;

    uint32_t palette_[64];
    bool pause_, step_, debug_, reset_;
    int stall_;
    uint64_t frame_;
};

}  // namespace protones
#endif // PROTONES_NES_NES_H


