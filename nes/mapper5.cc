#include "nes/mapper.h"

#include "nes/pbmacro.h"
#include "nes/cartridge.h"
#include "nes/ppu.h"
#include "nes/apu_pulse.h"

// This MMC5 implementation was developed from studying the nesdev MMC5
// document and studying the fceux MMC5 implementation.
//
// https://wiki.nesdev.com/w/index.php/MMC5

namespace protones {

class Mapper5: public Mapper {
  public:
    Mapper5(NES* nes):
        Mapper(nes),
        prg_banks_(nes_->cartridge()->prglen() / 0x2000),
        prg_mode_(0),
        chr_mode_(0),
        prg_ram_protect_{0},
        ext_ram_mode_(0),
        nt_map_(0),
        fill_tile_(0),
        fill_color_(0),
        prg_bank_{0,0,0,0,0xFF},
        chr_bank_{0},
        chr_upper_(0),
        vsplit_mode_(0),
        vsplit_scroll_(0),
        vsplit_bank_(0),
        vsplit_region_(false),
        irq_scanline_(0),
        scanline_counter_(0),
        irq_enable_(0),
        irq_status_(0),
        multiplier_{0xFF, 0xFF},
        ext_ram_{0},
        apu_divider_(0),
        cycle_(0),

        pulse_({{nes, 1}, {nes, 2}}),
        audio_debug_{&pulse_[0], &pulse_[1]}
        {
        pulse_[0].set_name("MMC5 Pulse 0");
        pulse_[1].set_name("MMC5 Pulse 1");
    }

    void LoadState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_mmc5();
        LOAD(prg_banks,
             prg_mode,
             chr_mode,
             ext_ram_mode,
             nt_map,
             fill_tile,
             fill_color,
             chr_upper,
             vsplit_mode,
             vsplit_scroll,
             vsplit_bank,
             vsplit_region,
             irq_scanline,
             scanline_counter,
             irq_enable,
             irq_status,
             apu_divider,
             cycle);
        LOAD_ARRAYS(prg_ram_protect,
                    prg_bank,
                    chr_bank,
                    multiplier);
        size_t len = std::min(sizeof(ext_ram_),
                              state->mutable_ext_ram()->size());
        memcpy(ext_ram_, state->mutable_ext_ram()->data(), len);
        pulse_[0].SaveState(state->mutable_pulse(0));
        pulse_[1].SaveState(state->mutable_pulse(1));
    }

    void SaveState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_mmc5();
        SAVE(prg_banks,
             prg_mode,
             chr_mode,
             ext_ram_mode,
             nt_map,
             fill_tile,
             fill_color,
             chr_upper,
             vsplit_mode,
             vsplit_scroll,
             vsplit_bank,
             vsplit_region,
             irq_scanline,
             scanline_counter,
             irq_enable,
             irq_status,
             apu_divider,
             cycle);
        SAVE_ARRAYS(prg_ram_protect,
                    prg_bank,
                    chr_bank,
                    multiplier);
        state->mutable_ext_ram()->assign(
                reinterpret_cast<const char*>(ext_ram_), sizeof(ext_ram_));

        state->clear_pulse();
        pulse_[0].SaveState(state->add_pulse());
        pulse_[1].SaveState(state->add_pulse());
    }

    inline uint8_t _prg_bank(size_t index) const {
        return (prg_bank_[index] & 0x7f) % prg_banks_;
    }
    inline bool rendering_enabled() const {
        return nes_->ppu()->mask().showsprites || nes_->ppu()->mask().showbg;
    }

    inline uint32_t TranslatePrg(uint16_t addr) {
        uint32_t result;
        uint32_t index;
        switch(prg_mode_) {
            case 0: // 1 x 32KiB mode
                result = (_prg_bank(4) * 8192) & ~0x7FFF;
                return result | (addr & 0x7FFF);
            case 1: // 2 x 16KiB mode
                index = (addr & 0x4000) ? 4 : 2;
                result = (_prg_bank(index) * 8192) & ~0x3FFF;
                return result | (addr & 0x3FFF);
            case 2: // 16 KiB + 8+8 mode
                if (addr & 0x4000) {
                    // 8+8 area
                    index = (addr & 0x2000) ? 4 : 3;
                    result = _prg_bank(index) * 8192;
                    result |= (addr & 0x1FFF);
                } else {
                    // 16KiB area
                    result = (_prg_bank(2) * 8192) & ~0x3FFF;
                    result |= (addr & 0x3FFF);
                }
                return result;
            case 3: // 8 KiB mode
                    index = 1 + ((addr >> 13) & 3);
                    result = _prg_bank(index) * 8192;
                    return result | (addr & 0x1FFF);
            default:
                fprintf(stderr, "Invalid MMC5 PRG mode %d\n", prg_mode_);
        }
        return 0;
    }

    inline uint32_t TranslateChr(uint16_t addr, bool bgbanks=false) {
        size_t regofs = 0;
        uint8_t mode = chr_mode_;
        if (bgbanks) {
            if (mode == 0) mode = 1;
            regofs = 8;
            addr &= 0x0FFF;
        }

        uint32_t result = chr_upper_ << 8;
        switch(mode) {
            case 0: // 8KiB mode
                result |= chr_bank_[regofs + 7];
                return (result << 13) | (addr & 0x1FFF);
            case 1: // 4KiB mode
                regofs |= (addr >> 12) * 4;
                result |= chr_bank_[regofs + 3];
                return (result << 12) | (addr & 0xFFF);
            case 2: // 2KiB mode
                regofs |= (addr >> 11) * 2;
                result |= chr_bank_[regofs + 1];
                return (result << 11) | (addr & 0x7FF);
            case 3: // 1KiB mode
                regofs |= (addr >> 10) * 1;
                result |= chr_bank_[regofs + 0];
                return (result << 10) | (addr & 0x3FF);
            default:
                fprintf(stderr, "Invalid MMC5 CHR mode %d\n", mode);
        }
        return 0;
    }

    void ReadChr2(uint16_t addr, uint8_t* a, uint8_t* b) override {
        bool bgbanks = nes_->ppu()->control().spritesize && rendering_enabled();
        uint32_t chraddr;
        if (vsplit_region_) {
            chraddr = (vsplit_bank_ * 4096) + (addr & 0x0FFF);
        } else {
            chraddr = TranslateChr(addr, bgbanks);
        }
        *a = nes_->cartridge()->ReadChr(chraddr);
        *b = nes_->cartridge()->ReadChr(chraddr + 8);
    }

    void ReadSpr2(uint16_t addr, uint8_t* a, uint8_t* b) override {
        uint32_t chraddr = TranslateChr(addr);
        *a = nes_->cartridge()->ReadChr(chraddr);
        *b = nes_->cartridge()->ReadChr(chraddr + 8);
    }

    virtual uint8_t* VramAddress(uint8_t* ppuram, uint16_t addr) override {
        static uint8_t junk;
        uint16_t offset = addr & 0x3FF;
        uint16_t table = (addr >> 10) & 3;
        uint8_t which = (nt_map_ >> (table * 2)) & 3;

        // Is vsplit enabled?
        if (vsplit_mode_ & 0x80) {
            uint16_t tile = vsplit_mode_ & 0x1f;
            // Re-compute PPU position from scanline/cycle data.
            int scanline = nes_->ppu()->scanline();
            int pputile = nes_->ppu()->cycle() + 15;
            // PPU fetches the next line's first two tiles during Hblank
            if (pputile >= 336) {
                pputile -= 336;
                scanline += 1;
            }
            pputile /= 8;
            int sofs = (scanline / 8) * 32 + pputile;
            if (offset >= 0x3c0) {
                // Attribute fetch.
                sofs = 0x3c0 | ((sofs >> 4) & 0x38) | ((sofs >> 2) & 7);
            }
            if (vsplit_mode_ & 0x40) {
                // Right side.
                if (pputile >= tile) {
                    vsplit_region_ = true;
                    return ext_ram_ + sofs;
                }
            } else {
                // Left side.
                if (pputile < tile) {
                    vsplit_region_ = true;
                    //return ext_ram_ + (noscroll & 0x3FF);
                    return ext_ram_ + sofs;
                }
            }
            vsplit_region_ = false;
        }

        switch(which) {
            case 0: return ppuram + offset;
            case 1: return ppuram + 0x400 + offset;
            case 2:
                if (ext_ram_mode_ <= 1) {
                    return ext_ram_ + offset;
                }
                junk = 0;
                break;
            case 3:
                if (offset < 0x3c0) {
                    junk = fill_tile_;
                } else {
                    junk = fill_color_;
                    junk |= junk << 2;
                    junk |= junk << 4;
                }
                break;
        }
        return &junk;
    }

    uint8_t Read(uint16_t addr) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->ReadChr(TranslateChr(addr));
        } else if (addr >= 0x5000 && addr < 0x5800) {
            return ReadRegister(addr);
        } else if (addr >= 0x5800 && addr < 0x5c00) {
            fprintf(stderr, "Unhandled MMC5 read at %04x\n", addr);
            return 0xff;
        } else if (addr >= 0x5c00 && addr < 0x6000) {
            //return (ext_ram_mode_ >= 2) ?  ext_ram_[addr - 0x5c00] : 0xFF;
            return ext_ram_[addr - 0x5c00];
        } else if (addr >= 0x6000 && addr < 0x8000) {
            uint32_t offset = (prg_bank_[0] * 8192) + (addr & 0x1FFF);
            return nes_->cartridge()->ReadSram(offset);
        } else if (addr >= 0x8000) {
            return nes_->cartridge()->ReadPrg(TranslatePrg(addr));
        } else {
            fprintf(stderr, "Unhandled MMC5 read at %04x\n", addr);
        }
        return 0;
    }

    void Write(uint16_t addr, uint8_t val) override {
        if (addr < 0x2000) {
            return nes_->cartridge()->WriteChr(addr, val);
        } else if (addr >= 0x5000 && addr < 0x5800) {
            return WriteRegister(addr, val);
        } else if (addr >= 0x5800 && addr < 0x5c00) {
            fprintf(stderr, "Unhandled MMC5 write at %04x\n", addr);
        } else if (addr >= 0x5c00 && addr < 0x6000) {
            ext_ram_[addr - 0x5c00] = val;
        } else if (addr >= 0x6000 && addr < 0x8000) {
            bool enabled = prg_ram_protect_[0] == 2 &&
                           prg_ram_protect_[1] == 1;
            if (enabled) {
                uint32_t offset = (prg_bank_[0] * 8192) + (addr & 0x1FFF);
                nes_->cartridge()->WriteSram(offset, val);
            }
        } else if (addr >= 0x8000) {
            // TODO: depends on prg mapping and WP value.
            fprintf(stderr, "Unhandled MMC5 PRG write at %04x\n", addr);
        } else {
            fprintf(stderr, "Unhandled MMC5 write at %04x\n", addr);
        }
    }

    uint8_t ReadRegister(uint16_t addr) {
        uint16_t product;
        uint8_t val;
        switch(addr) {
            case 0x5100:
                return prg_mode_;
            case 0x5101:
                return chr_mode_;
            case 0x5102 ... 0x5103:
                return prg_ram_protect_[addr - 0x5102];
            case 0x5104:
                return ext_ram_mode_;
            case 0x5105:
                return nt_map_;
            case 0x5106:
                return fill_tile_;
            case 0x5107:
                return fill_color_;
            case 0x5113 ... 0x5117:
                return prg_bank_[addr - 0x5113];
            case 0x5120 ... 0x512b:
                return chr_bank_[addr - 0x5120];
            case 0x5130:
                return chr_upper_;
            case 0x5200:
                return vsplit_mode_;
            case 0x5201:
                return vsplit_scroll_;
            case 0x5202:
                return vsplit_bank_;
            case 0x5203:
                return irq_scanline_;
            case 0x5204:
                val = irq_status_;
                // Clear pending flag.
                irq_status_ &= ~0x80;
                return val;
            case 0x5205 ... 0x5206:
                product = multiplier_[0] * multiplier_[1];
                return addr == 0x5205 ? uint8_t(product)
                                      : uint8_t(product >> 8);

            default:
                // Unhandled MMC5 register read.
                fprintf(stderr, "Unhandled MMC5 register read at %04x\n", addr);
                return 0xff;
        }
    }

    void WriteRegister(uint16_t addr, uint8_t val) {
        //printf("MMC5 Reg: %04x = %02x\n", addr, val);
        switch(addr) {
            case 0x5000: pulse_[0].set_control(val); break;
            case 0x5001: break; // MMC5 pulse channels have no sweep unit.
            case 0x5002: pulse_[0].set_timer_low(val); break;
            case 0x5003: pulse_[0].set_timer_high(val); break;

            case 0x5004: pulse_[1].set_control(val); break;
            case 0x5005: break; // MMC5 pulse channels have no sweep unit.
            case 0x5006: pulse_[1].set_timer_low(val); break;
            case 0x5007: pulse_[1].set_timer_high(val); break;
            case 0x5015:
                pulse_[0].set_enabled(!!(val & 0x01));
                pulse_[1].set_enabled(!!(val & 0x02));
                break;

            case 0x5100:
                prg_mode_ = val & 0x03; break;
            case 0x5101:
                chr_mode_ = val & 0x03; break;
            case 0x5102 ... 0x5103:
                prg_ram_protect_[addr - 0x5102] = val & 0x03;
                break;
            case 0x5104:
                ext_ram_mode_ = val & 0x03; break;
            case 0x5105:
                nt_map_ = val; break;
            case 0x5106:
                fill_tile_ = val; break;
            case 0x5107:
                fill_color_ = val & 0x03; break;
            case 0x5113 ... 0x5117:
                prg_bank_[addr - 0x5113] = val;
                break;
            case 0x5120 ... 0x512b:
                chr_bank_[addr - 0x5120] = val;
                break;
            case 0x5130:
                chr_upper_ = val & 0x03; break;
            case 0x5200:
                vsplit_mode_ = val & 0xDF;
                vsplit_region_ = false;
                break;
            case 0x5201:
                vsplit_scroll_ = val; break;
            case 0x5202:
                vsplit_bank_ = val; break;
            case 0x5203:
                irq_scanline_ = val; break;
            case 0x5204:
                irq_enable_ = val & 0x80; break;
            case 0x5205 ... 0x5206:
                multiplier_[addr - 0x5205] = val;
            default:
                // Unhandled MMC5 register write.
                fprintf(stderr,
                        "Unhandled MMC5 register write at %04x (val=%02x)\n",
                        addr, val);
        }
    }

    void CheckScanline() {
        if (!rendering_enabled() || nes_->ppu()->scanline() > 240) {
            // Clear irq_pending and in_frame.
            irq_status_ &= ~0xC0;
            scanline_counter_ = 0;
            return;
        }
        if (irq_status_ & 0x40) {
            // If in_frame and the count is equal, signal an interrupt.
            scanline_counter_ += 1;
            if (scanline_counter_ == irq_scanline_) {
                irq_status_ |= 0x80;
                if (irq_enable_) {
                    nes_->IRQ();
                }
            }
        } else {
            // Not in_frame, so become in_inframe.
            irq_status_ = (irq_status_ | 0x40) & ~0x80;
            scanline_counter_ = 0;
        }

    }

    void Emulate() override {
        // According to the nesdev MMC5 document, the IRQ should happen
        // at ppu cycle 260.
        if (nes_->ppu()->cycle() == 0) {
            CheckScanline();
        }
        // The mapper is clocked at the PPU clock rate.
        if (apu_divider_++ == 2) {
            // Re-derive the cpu clock to clock the pulse channels.
            apu_divider_ = 0;
            EmulateAudio();
        }
    }

    void EmulateAudio() {
        double c1 = double(cycle_);
        double c2 = double(++cycle_);
        if (cycle_ % 2 == 0) {
            // Pulse channels are clocked at half the cpu rate.
            pulse_[0].StepTimer();
            pulse_[1].StepTimer();
        }

        int f1 = int(c1 / NES::frame_counter_rate);
        int f2 = int(c2 / NES::frame_counter_rate);
        if (f1 == f2) {
            return;
        }
        // In MMC5, the pulse envelopes and lengths are clocked at 240 Hz.
        pulse_[0].StepEnvelope();
        pulse_[0].StepLength();
        pulse_[1].StepEnvelope();
        pulse_[1].StepLength();
    }

    float ExpansionAudio() override {
        float p0 = pulse_[0].Output();
        float p1 = pulse_[1].Output();
        return p0 + p1;
    }
    const APUDevices& DebugExpansionAudio() override { return audio_debug_; }

  private:
    uint8_t prg_banks_;
    uint8_t prg_mode_;
    uint8_t chr_mode_;
    uint8_t prg_ram_protect_[2];
    uint8_t ext_ram_mode_;
    uint8_t nt_map_;
    uint8_t fill_tile_;
    uint8_t fill_color_;
    uint8_t prg_bank_[5];
    uint8_t chr_bank_[12];
    uint8_t chr_upper_;

    uint8_t vsplit_mode_;
    uint8_t vsplit_scroll_;
    uint8_t vsplit_bank_;
    bool vsplit_region_;

    uint8_t irq_scanline_;
    uint8_t scanline_counter_;
    uint8_t irq_enable_;
    uint8_t irq_status_;

    uint8_t multiplier_[2];

    // "Extended" ram.
    uint8_t ext_ram_[1024];

    uint32_t apu_divider_;
    uint32_t cycle_;

    Pulse pulse_[2];
    APUDevices audio_debug_;
};

REGISTER_MAPPER(5, Mapper5);
}  // namespace protones
