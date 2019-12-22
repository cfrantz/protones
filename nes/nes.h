#ifndef PROTONES_NES_NES_H
#define PROTONES_NES_NES_H
#include <string>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>

#include "nes/base.h"
#include "proto/nes.pb.h"
#include "proto/controller.pb.h"

namespace protones {

class APU;
class Cpu;
class Cartridge;
class Controller;
class Debugger;
class FM2Movie;
class Mapper;
class Mem;
class PPU;
class MidiConnector;

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
    inline FM2Movie* movie() { return movie_; }
    inline Mapper* mapper() { return mapper_; }
    inline Cartridge* cartridge() { return cart_; }
    inline Controller* controller(int n) { return controller_[n]; }
    inline MidiConnector* midi() { return midi_; }
    inline uint32_t palette(uint8_t c) { return palette_[c % 64]; }
    inline uint64_t frame() { return frame_; }
    inline bool lag() { return lag_; }
    inline void set_lag(bool val) { lag_ = val; }
    inline bool has_movie() { return has_movie_; }
    inline bool pause() { return pause_; }
    inline void set_pause(bool p) { pause_ = p; }
    inline const std::map<int, int>& frame_profile() { return frame_profile_; }

    uint64_t cpu_cycles();
    void Stall(int s);

    void Reset();
    bool Emulate();
    bool EmulateFrame();
    void HandleKeyboard(SDL_Event* event);

    bool LoadState(const std::string& state);
    std::string SaveState(bool text=false);

    bool LoadStateFromFile(const std::string& filename);
    bool SaveStateToFile(const std::string& filename, bool text=false);
    bool LoadEverdriveStateFromFile(const std::string& filename);

    static const int frequency = 1789773;
    static constexpr double frame_counter_rate = frequency / 240.0;
    static constexpr double sample_rate = frequency / 44100.0;
  private:
    void DebugPalette(bool* active);
    APU* apu_;
    Cpu* cpu_;
    FM2Movie* movie_;
    PPU* ppu_;
    Mem* mem_;
    Mapper* mapper_;
    Cartridge* cart_;
    Controller* controller_[4];
    MidiConnector* midi_;
    std::vector<std::unique_ptr<EmulatedDevice>> devices_;

    proto::NES state_;

    uint32_t palette_[64];
    bool pause_, step_, debug_, reset_, lag_, has_movie_;
    uint64_t frame_;
    double remainder_;
    std::map<int, proto::ControllerButtons> buttons_;
    std::map<int, int> frame_profile_;
};

}  // namespace protones
#endif // PROTONES_NES_NES_H


