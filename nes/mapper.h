#ifndef PROTONES_NES_MAPPER_H
#define PROTONES_NES_MAPPER_H
#include <functional>
#include <map>
#include <cstdint>
#include "nes/base.h"
#include "nes/cartridge.h"
#include "nes/nes.h"
#include "proto/mappers.pb.h"
namespace protones {

class Mapper : public EmulatedDevice {
  public:
    using APUDevices = std::vector<APUDevice*>;

    Mapper(NES* nes) : nes_(nes) {}
    virtual uint8_t Read(uint16_t addr) = 0;
    virtual void ReadChr2(uint16_t addr, uint8_t* a, uint8_t* b) {
        *a = Read(addr);
        *b = Read(addr + 8);
    }

    // Helper function for MMC5 implementation.
    virtual void ReadSpr2(uint16_t addr, uint8_t* a, uint8_t* b) {
        ReadChr2(addr, a, b);
    }

    virtual void Write(uint16_t addr, uint8_t val) = 0;
    virtual void Emulate() {}
    virtual void LoadState(proto::Mapper *state) {}
    virtual void SaveState(proto::Mapper *state) {}
    virtual void LoadEverdriveState(const uint8_t* state) {}
    virtual uint8_t& RegisterValue(int reg) {
        static uint8_t bogus = 0xff;
        return bogus;
    }

    // Calculate addresses based on the "standard" mirror modes for the NES.
    uint16_t MirrorAddress(uint16_t addr) {
        static const uint16_t lookup[5][4] = {
            { 0, 0, 1, 1, }, // Horiz
            { 0, 1, 0, 1, }, // Vert
            { 0, 0, 0, 0, }, // single 0
            { 1, 1, 1, 1, }, // single 1
            { 0, 1, 2, 3, }, // four
        };
        addr = (addr - 0x2000) % 0x1000;
        int table = addr / 0x400;
        int offset = addr % 0x400;
        int mode = int(nes_->cartridge()->mirror());
        return lookup[mode][table] * 0x400 + offset;
    }

    // Translate a PPU VRAM address based on the mirroring mode.
    // Advanced mappers like MMC5 can change how they deal with vram.
    virtual uint8_t* VramAddress(uint8_t* ppuram, uint16_t addr) {
        return ppuram + MirrorAddress(addr);
    }

    virtual float ExpansionAudio() { return 0; }
    virtual const APUDevices& DebugExpansionAudio() {
        static APUDevices empty;
        return empty;
    }

  protected:
    NES* nes_;
};

class MapperRegistry {
  public:
    MapperRegistry(int n, std::function<Mapper*(NES*)> create);
    static Mapper* New(NES* nes, int n);
  private:
    static std::map<int, std::function<Mapper*(NES*)>>* mappers();
};

#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)

#define REGISTER_MAPPER(n_, type_) \
    MapperRegistry CONCAT(CONCAT(CONCAT(reg_, type_), __), __LINE__) (n_, \
            [](NES* nes) { return new type_(nes); })

}  // namespace protones
#endif // PROTONES_NES_MAPPER_H
