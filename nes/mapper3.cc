#include "nes/mapper.h"

#include "nes/pbmacro.h"
#include "nes/cartridge.h"
namespace protones {

class Mapper3: public Mapper {
  public:
    Mapper3(NES* nes):
        Mapper(nes),
        chr_banks_(nes_->cartridge()->chrlen() / 0x2000),
        chr_bank1_(0) {}

    void LoadState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_cnrom();
        LOAD(chr_banks, chr_bank1);
    }

    void SaveState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_cnrom();
        SAVE(chr_banks, chr_bank1);
    }

    uint8_t Read(uint16_t addr) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->ReadChr(chr_bank1_*0x2000 + addr);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->ReadSram(addr - 0x6000);
        } else {
            return nes_->cartridge()->ReadPrg(addr-0x8000);
        }
        return 0;
    }

    void Write(uint16_t addr, uint8_t val) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->WriteChr(chr_bank1_*0x2000 + addr, val);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->WriteSram(addr - 0x6000, val);
        } else if (addr >= 0x8000) {
            chr_bank1_ = val & 3;
        } else {
            fprintf(stderr, "Unhandled mapper3 write at %04x\n", addr);
        }
    }
    void LoadEverdriveState(const uint8_t* state) override {
        chr_bank1_ = state[0x7000];
    }

  private:
    uint8_t chr_banks_, chr_bank1_;
};

REGISTER_MAPPER(3, Mapper3);
}  // namespace protones
