#ifndef PROTONES_NES_MEM_H
#define PROTONES_NES_MEM_H
#include <string>
#include <vector>

#include "nes/base.h"
#include "nes/nes.h"
#include "proto/nes.pb.h"
namespace protones {

class Mem : public EmulatedDevice {
  public:
    Mem(NES* nes);
    ~Mem() {}

    inline uint8_t Read(uint16_t addr) { return read_byte(addr); }
    inline void Write(uint16_t addr, int val) { return write_byte(addr, val); }

    virtual uint8_t read_byte(uint16_t addr) ;
    virtual uint8_t read_byte_no_io(uint16_t addr) ;
    virtual void write_byte(uint16_t addr, uint8_t v) ;
    virtual void write_byte_no_io(uint16_t addr, uint8_t v) ;
    uint16_t read_word(uint16_t addr) ;
    uint16_t read_word_no_io(uint16_t) ;
    void write_word(uint16_t addr, uint16_t v) ;
    void write_word_no_io(uint16_t addr, uint16_t v) ;

    uint8_t PPURead(uint16_t addr);
    void PPUWrite(uint16_t addr, uint8_t val);

    inline uint8_t PaletteRead(uint16_t addr) {
        if (addr >= 16 && (addr % 4) == 0)
            addr -= 16;
        return palette_[addr];
    }

    inline void PaletteWrite(uint16_t addr, uint8_t val) {
        if (addr >= 16 && (addr % 4) == 0)
            addr -= 16;
        palette_[addr] = val;
    }

    void LoadState(proto::NES* state);
    void SaveState(proto::NES* state);
    void LoadEverdriveState(const uint8_t* state);

  private:
    uint16_t MirrorAddress(int mode, uint16_t addr);

    NES* nes_;
    uint8_t ram_[2048];
    // NES has 2k of PPU vram, some carts provide extra vram.
    uint8_t ppuram_[4096];
    uint8_t palette_[32];

    std::vector<std::string> custom_memdump_;
};

}  // namespace protones
#endif // PROTONES_NES_MEM_H
