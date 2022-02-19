#include "nes/mapper.h"

#include "nes/base.h"
#include "nes/pbmacro.h"
#include "nes/cpu6502.h"
#include "nes/cartridge.h"
#include "nes/vrc7_audio/emu2413.h"

namespace protones {

class VRC7Audio: public APUDevice {
  public:
    VRC7Audio()
      : APUDevice(nullptr, 0.25) {}
    VRC7Audio(const char* name, float volume)
      : APUDevice(name, volume) {}

    Type type() const override {
        return Type::VRC7;
    }
    void set_sample(float val) {
        dbgbuf_[dbgp_] = val;
        dbgp_ = (dbgp_ + 1) % DBGBUFSZ;
    }

    const char *text(size_t n) const override {
        if (n < 2) {
            return status_[n];
        }
        return nullptr;
    }
    char status_[2][64];
};

class VRC7: public Mapper {
  public:
    VRC7(NES* nes):
        Mapper(nes),
        prg_banks_(nes_->cartridge()->prglen() / 0x2000),
        prg_bank_{0,},
        chr_banks_(nes_->cartridge()->chrlen() / 0x400),
        chr_bank_{0,},
        mirror_(0),
        irq_latch_(0),
        irq_control_(0),
        irq_counter_(0),
        cycle_counter_(0),
        oplidx_(0),
        opl_(OPLL_new(3579545, 44100)),
        audio_debug_{
            &audio_[0],
            &audio_[1],
            &audio_[2],
            &audio_[3],
            &audio_[4],
            &audio_[5],
        }
    {
        audio_[0].set_name("VRC7:0");
        audio_[1].set_name("VRC7:1");
        audio_[2].set_name("VRC7:2");
        audio_[3].set_name("VRC7:3");
        audio_[4].set_name("VRC7:4");
        audio_[5].set_name("VRC7:5");
    }

    void LoadState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_vrc7();
        LOAD(prg_banks,
             chr_banks,
             mirror,
             irq_latch,
             irq_control,
             irq_counter,
             cycle_counter,
             oplidx);
        LOAD_ARRAYS(prg_bank, chr_bank);
    }

    void SaveState(proto::Mapper* mstate) {
        auto* state = mstate->mutable_vrc7();
        SAVE(prg_banks,
             chr_banks,
             mirror,
             irq_latch,
             irq_control,
             irq_counter,
             cycle_counter,
             oplidx);
        SAVE_ARRAYS(prg_bank, chr_bank);
    }

    uint8_t Read(uint16_t addr) override {
        if (addr < 0x2000) {
            uint32_t n = addr / 0x400;
            uint32_t base = (chr_bank_[n] % chr_banks_) * 1024;
            return nes_->cartridge()->ReadChr(base + addr % 0x400);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            return mirror_ & 0x80
                ?  nes_->cartridge()->ReadSram(addr - 0x6000)
                : 0xFF;
        } else if (addr >= 0x8000 && addr < 0xA000) {
            uint32_t base = (prg_bank_[0] % prg_banks_) * 8192;
            return nes_->cartridge()->ReadPrg(base + addr % 8192);
        } else if (addr >= 0xA000 && addr < 0xC000) {
            uint32_t base = (prg_bank_[1] % prg_banks_) * 8192;
            return nes_->cartridge()->ReadPrg(base + addr % 8192);
        } else if (addr >= 0xC000 && addr < 0xE000) {
            uint32_t base = (prg_bank_[2] % prg_banks_) * 8192;
            return nes_->cartridge()->ReadPrg(base + addr % 8192);
        } else if (addr >= 0xE000) {
            uint32_t base = (prg_banks_ - 1) * 8192;
            return nes_->cartridge()->ReadPrg(base + addr % 8192);
        } else {
            fprintf(stderr, "Unhandled VRC7 read at %04x\n", addr);
        }
        return 0;
    }

    void WriteAudio(uint8_t idx, uint8_t val) {
        const char *instruments[] = {
            // Name in FamiTracker, Name in https://wiki.nesdev.org/w/index.php?title=VRC7_audio
            "Custom",               // 0
            "Bell",                 // 1 "Buzzy Bell",
            "Guitar",               // 2
            "Piano",                // 3 "Wurly",
            "Flute",                // 4
            "Clarinet",             // 5
            "Rattling Bell",        // 6 "Synth",
            "Trumpet",              // 7
            "Organ",                // 8
            "Soft Bell",            // 9 "Bells",
            "Xylophone",            // 10 "Vibes",
            "Vibraphone",           // 11
            "Brass",                // 12 "Tutti",
            "Bass Guitar",          // 13 "Fretless",
            "Synthesizer",          // 14 "Synth Bass",
            "Chorus",               // 15 "Sweep",
        };
        OPLL_writeReg(opl_, oplidx_, val);
        if (idx >= 0x30 && idx <= 0x35) {
            idx &= 0x07;
            snprintf(audio_[idx].status_[0], sizeof(audio_[idx].status_[0]),
                    "Volume: 0x%02x", opl_->InstVol[idx] & 0x0F);
            snprintf(audio_[idx].status_[1], sizeof(audio_[idx].status_[1]),
                    "Instrument: %s(%d)",
                    instruments[opl_->patch_number[idx]],
                    opl_->patch_number[idx]);
        }
    }

    void Write(uint16_t addr, uint8_t val) override {
        if (addr < 0x2000) {
            uint32_t n = addr / 0x400;
            uint32_t base = (chr_bank_[n] % chr_banks_) * 1024;
            return nes_->cartridge()->WriteChr(base + addr % 0x400, val);
        } else if (addr >= 0x6000 && addr < 0x8000) {
            if (mirror_ & 0x80) {
                return nes_->cartridge()->WriteSram(addr - 0x6000, val);
            }
        } else if (addr >= 0x8000) {
            uint16_t a = addr | ((addr & 0x08) << 1);
            switch (a & 0xF030) {
                case 0x8000: prg_bank_[0] = val; break;
                case 0x8010: prg_bank_[1] = val; break;
                case 0x9000: prg_bank_[2] = val; break;
                case 0x9010: oplidx_ = val; break;
                case 0x9030:
                     WriteAudio(oplidx_, val);
                     break;
                case 0xA000: chr_bank_[0] = val; break;
                case 0xA010: chr_bank_[1] = val; break;
                case 0xB000: chr_bank_[2] = val; break;
                case 0xB010: chr_bank_[3] = val; break;
                case 0xC000: chr_bank_[4] = val; break;
                case 0xC010: chr_bank_[5] = val; break;
                case 0xD000: chr_bank_[6] = val; break;
                case 0xD010: chr_bank_[7] = val; break;
                case 0xE000:
                     mirror_ = val & 0xC3;
                     switch(val & 0x03) {
                         case 0:
                             nes_->cartridge()->set_mirror(Cartridge::MirrorMode::VERTICAL);
                             break;
                         case 1:
                             nes_->cartridge()->set_mirror(Cartridge::MirrorMode::HORIZONTAL);
                             break;
                         case 2:
                             nes_->cartridge()->set_mirror(Cartridge::MirrorMode::SINGLE0);
                             break;
                         case 3:
                             nes_->cartridge()->set_mirror(Cartridge::MirrorMode::SINGLE1);
                             break;
                     }
                     break;
                case 0xE010: irq_latch_ = val; break;
                case 0xF000:
                    irq_control_ = val;
                    cycle_counter_ = 0;
                    if (val & 2) {
                        irq_counter_ = irq_latch_;
                    }
                    break;
                case 0xF010: {
                    uint8_t eaa = (irq_control_ & 1) << 1;
                    irq_control_ &= ~0x02;
                    irq_control_ |= eaa;
                    }
                    break;
                default:
                    goto unhandled;
            }
        } else {
unhandled:
            fprintf(stderr, "Unhandled VRC7 write at %04x\n", addr);
        }
    }

    void Emulate() {
        if (irq_control_ & 2) {
            cycle_counter_++;
            while (cycle_counter_ >= 341) {
                cycle_counter_ -= 341;
                irq_counter_++;
                if (irq_counter_ == 0x100) {
                    irq_counter_ = irq_latch_;
                    nes_->IRQ();
                }
            }
        }
    }

    float ExpansionAudio() override {
        float output[6], sum=0.0;
        OPLL_output(opl_, output);
        for(size_t i=0; i<6; i++) {
            output[i] *= 32.0;
            audio_[i].set_sample(output[i]);
            sum += output[i] * audio_[i].output_volume();
        }
        return sum;
/*
        int32_t sample = 0;
        OPLL_fillbuf(opl_, &sample, 1, 0);
        float val = float(sample - 32768) / 65535.0;
        val *= 32.0;
        audio_.set_sample(val);
        return val * audio_.output_volume();
        */
    }
    const APUDevices& DebugExpansionAudio() override { return audio_debug_; }

    uint8_t RegisterValue(PseudoRegister reg) {
        switch(reg) {
            case PseudoRegister::CpuExecBank: {
                uint16_t pc = nes_->cpu()->pc();
                switch(pc & 0xE000) {
                    case 0x8000: return prg_bank_[0] % prg_banks_;
                    case 0xA000: return prg_bank_[1] % prg_banks_;
                    case 0xC000: return prg_bank_[2] % prg_banks_;
                    case 0xE000: return prg_banks_-1;
                    default:
                        fprintf(stderr, "Unknown PRG bank for pc=%04x\n", pc);
                        return 0;
                }
            }
            default:
                return Mapper::RegisterValue(reg);
        }
    }

  private:
    uint8_t prg_banks_, prg_bank_[3];
    uint8_t chr_banks_, chr_bank_[8];
    uint8_t mirror_;
    uint8_t irq_latch_, irq_control_;
    int32_t irq_counter_, cycle_counter_;
    uint8_t oplidx_;
    OPLL *opl_;
    VRC7Audio audio_[6];
    APUDevices audio_debug_;
};

REGISTER_MAPPER(85, VRC7);
}  // namespace protones
