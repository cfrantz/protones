#include "nes/mapper.h"

#include "nes/pbmacro.h"
#include "nes/cartridge.h"
namespace protones {

class Mapper2: public Mapper {
  public:
    Mapper2(NES* nes):
        Mapper(nes),
        prg_banks_(nes_->cartridge()->prglen() / 0x4000),
        prg_bank1_(0),
        prg_bank2_(prg_banks_ - 1) {}

    void LoadState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_unrom();
        LOAD(prg_banks, prg_bank1, prg_bank2);
    }

    void SaveState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_unrom();
        SAVE(prg_banks, prg_bank1, prg_bank2);
    }

    uint8_t Read(uint16_t addr) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->ReadChr(addr);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->ReadSram(addr - 0x6000);
        } else if (addr < 0xC000) {
            return nes_->cartridge()->ReadPrg(prg_bank1_*0x4000 + addr-0x8000);
        } else if (addr >= 0xC000) {
            return nes_->cartridge()->ReadPrg(prg_bank2_*0x4000 + addr-0xC000);
        } else {
            fprintf(stderr, "Unhandled mapper2 read at %04x\n", addr);
        }
        return 0;
    }

    void Write(uint16_t addr, uint8_t val) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->WriteChr(addr, val);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->WriteSram(addr - 0x6000, val);
        } else if (addr >= 0x8000) {
            prg_bank1_ = val % prg_banks_;
        } else {
            fprintf(stderr, "Unhandled mapper2 write at %04x\n", addr);
        }
    }

  private:
    int prg_banks_, prg_bank1_, prg_bank2_;
};

REGISTER_MAPPER(0, Mapper2);
REGISTER_MAPPER(2, Mapper2);
}  // namespace protones
