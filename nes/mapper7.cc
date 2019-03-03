#include "nes/mapper.h"

#include "nes/pbmacro.h"
#include "nes/cartridge.h"
namespace protones {

class Mapper7: public Mapper {
  public:
    Mapper7(NES* nes):
        Mapper(nes),
        prg_banks_(nes_->cartridge()->prglen() / 0x8000),
        prg_bank1_(0) {}

    void LoadState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_axrom();
        LOAD(prg_banks, prg_bank1);
    }

    void SaveState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_axrom();
        SAVE(prg_banks, prg_bank1);
    }

    uint8_t Read(uint16_t addr) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->ReadChr(addr);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->ReadSram(addr - 0x6000);
        } else if (addr >= 0x8000) {
            uint32_t mask = (1UL << prg_banks_) - 1;
            uint32_t offset = (prg_bank1_ & mask) * 0x8000;
            return nes_->cartridge()->ReadPrg(offset + addr-0x8000);
        } else {
            fprintf(stderr, "Unhandled mapper7 read at %04x\n", addr);
        }
        return 0;
    }

    void Write(uint16_t addr, uint8_t val) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->WriteChr(addr, val);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return nes_->cartridge()->WriteSram(addr - 0x6000, val);
        } else if (addr >= 0x8000) {
            prg_bank1_ = val;
        } else {
            fprintf(stderr, "Unhandled mapper7 write at %04x\n", addr);
        }
    }

  private:
    uint8_t prg_banks_;
    uint8_t prg_bank1_;
};

REGISTER_MAPPER(7, Mapper7);
}  // namespace protones
