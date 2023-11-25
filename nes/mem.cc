#include <fstream>

#include "nes/mem.h"
#include "nes/pbmacro.h"

#include "absl/flags/flag.h"
#include "nes/cpu6502.h"
#include "nes/apu.h"
#include "nes/cartridge.h"
#include "nes/controller.h"
#include "nes/mapper.h"
#include "nes/ppu.h"

ABSL_FLAG(std::string, memdump, "", "Custom memory dump textfile.");
namespace protones {

Mem::Mem(NES* nes)
    : nes_(nes),
      ram_{0, },
      ppuram_{0, } {
}

void Mem::LoadState(proto::NES* state) {
    const auto& ram = state->ram();
    const auto& ppuram = state->ppu().ppuram();
    const auto& palette = state->ppu().palette();
    memcpy(ram_, ram.data(),
           ram.size() <= sizeof(ram_) ? ram.size() : sizeof(ram_));
    memcpy(ppuram_, ppuram.data(),
           ppuram.size() <= sizeof(ppuram_) ? ppuram.size() : sizeof(ppuram_));
    memcpy(palette_, palette.data(),
           palette.size() <= sizeof(palette_) ? palette.size() : sizeof(palette_));
}

void Mem::LoadEverdriveState(const uint8_t* state) {
    memcpy(ram_, state+0x6800, 0x800);
    memcpy(ppuram_, state+0x6000, 0x800);
    memcpy(palette_, state+0x7100, 16);
}

void Mem::SaveState(proto::NES* state) {
    auto* ram = state->mutable_ram();
    auto* ppuram = state->mutable_ppu()->mutable_ppuram();
    auto* palette = state->mutable_ppu()->mutable_palette();

    ram->assign((char*)ram_, sizeof(ram_));
    ppuram->assign((char*)ppuram_, sizeof(ppuram_));
    palette->assign((char*)palette_, sizeof(palette_));
}


uint8_t Mem::read_byte(uint16_t addr) {
    if (addr < 0x2000) {
        return ram_[addr];
    } else if (addr < 0x4000 || addr == 0x4014) {
        return nes_->ppu()->Read(addr);
    } else if (addr == 0x4015) {
        return nes_->apu()->Read(addr);
    } else if (addr == 0x4016) {
        return nes_->controller(0)->Read();
    } else if (addr == 0x4017) {
        return nes_->controller(1)->Read();
    } else if (addr == 0x4019) {
        static uint64_t t0;
        uint64_t t1 = nes_->cpu()->cycles();
        if (t0) {
            unsigned diff = t1 - t0;
            printf("Elapsed: %d cpu cycles (%.6f s)\n", diff,
                   double(diff) / NES::frequency);
            t0 = 0;
        } else {
            t0 = t1;
        }
        return 0;
    } else if (addr == 0x401b) {
        static uint64_t t0;
        uint64_t t1 = nes_->cpu()->cycles();
        unsigned diff = t1 - t0;
        printf("Elapsed: %d cpu cycles (%.6f s)\n", diff,
               double(diff) / NES::frequency);
        t0 = t1;
        return 0;

    } else if (addr >= 0x5000) {
        return nes_->mapper()->Read(addr);
    } else {
        fprintf(stderr, "Unknown read at %04x\n", addr);
    }
    return 0;
}

uint8_t Mem::read_byte_no_io(uint16_t addr) {
    if (addr < 0x2000) {
        return ram_[addr];
    } else if (addr < 0x4000) {
        return PPURead(addr);
    } else if (addr >= 0x5c00 && addr < 0x6000) {
        if (nes_->cartridge()->mapper() == 5) {
            return nes_->mapper()->Read(addr);
        }
        return 0;
    } else if (addr >= 0x6000) {
        return nes_->mapper()->Read(addr);
    } else {
    }
    return 0;
}

void Mem::write_byte(uint16_t addr, uint8_t v) {
    if (addr < 0x2000) {
        ram_[addr] = v;
    } else if (addr < 0x4000 || addr == 0x4014) {
        return nes_->ppu()->Write(addr, v);
    } else if (addr == 0x4016) {
        nes_->controller(0)->Write(v);
        nes_->controller(1)->Write(v);
    } else if (addr >= 0x4000 && addr <= 0x4017) {
        nes_->apu()->Write(addr, v);
    } else if (addr == 0x4018) {
        fputc(v, stdout);
    } else if (addr == 0x401a) {
        printf("hexout = %02x\n", v);
    } else if ((addr & 0xFF00) == 0x4100) {
        int n = (addr & 0x00FF) / 2;
        if (addr & 1) {
            uint64_t tm = nes_->cpu_cycles() - counters_[n];
            double percent = 100.0 * double(tm) / 29780.0;
            printf("Cycle Counter %d: %6" PRIu64 " (%.02f%%)\n", n, tm, percent);
        } else {
            counters_[n] = nes_->cpu_cycles();
        }
    } else if (addr >= 0x5000) {
        nes_->mapper()->Write(addr, v);
    } else {
        fprintf(stderr, "Unknown write at %04x = %02x\n", addr, v);
    }
}

void Mem::write_byte_no_io(uint16_t addr, uint8_t v) {
}

uint16_t Mem::read_word(uint16_t addr) {
    uint16_t result = read_byte(addr);
    result |= read_byte(addr+1) << 8;
    return result;
}

uint16_t Mem::read_word_no_io(uint16_t) {
    return 0;
}
void Mem::write_word(uint16_t addr, uint16_t v) {
}
void Mem::write_word_no_io(uint16_t addr, uint16_t v) {
}

uint8_t Mem::PPURead(uint16_t addr) {
    addr %= 0x4000;
    if (addr < 0x2000) {
        return nes_->mapper()->Read(addr);
    } else if (addr < 0x3F00) {
        return *nes_->mapper()->VramAddress(ppuram_, addr);
    } else {
        return PaletteRead(addr % 32);
    }
}

void Mem::PPUWrite(uint16_t addr, uint8_t val) {
    addr %= 0x4000;
    if (addr < 0x2000) {
        nes_->mapper()->Write(addr, val);
    } else if (addr < 0x3F00) {
        *nes_->mapper()->VramAddress(ppuram_, addr) = val;
    } else {
        PaletteWrite(addr % 32, val);
    }
}

uint8_t Mem::CpuExecBank() {
    return nes_->mapper()->RegisterValue(Mapper::PseudoRegister::CpuExecBank);
}
#ifdef DELETE_ME
#include "imgui.h"
void Mem::HexDump(int addr, int len) {
    static const char hex[] = "0123456789abcdef";
    char line[80];
    char *b = line;
    int i, c=55;
    uint8_t val;

    for(i=0; i < len; i++) {
        uint16_t a = addr + i;
        val = read_byte_no_io(a);
        if (i % 16 == 0) {
            if (i) {
                line[c] = '\0';
                ImGui::Text("%s", line);
            }
            memset(line, 32, sizeof(line));
            b = line; c = 55;
            *b++ = hex[(a>>12) & 0xf];
            *b++ = hex[(a>>8) & 0xf];
            *b++ = hex[(a>>4) & 0xf];
            *b++ = hex[(a>>0) & 0xf];
            *b++ = ':';
        }
        int k = i%16;
        if (k==4 || k==8 || k==12) {
            *b++ = '-';
        } else {
            *b++ = ' ';
        }
        *b++ = hex[(val>>4) & 0xf];
        *b++ = hex[(val>>0) & 0xf];
        line[c++] = (val>=32 && val<127) ? val : '.';
    }
    line[c] = '\0';
    ImGui::Text("%s", line);
}

bool Mem::ReadMemDump() {
    static bool once;
    if (absl::GetFlag(FLAGS_memdump).empty())
        return false;
    if (once)
        return true;

    std::ifstream input(absl::GetFlag(FLAGS_memdump));
    std::string line;

    while(getline(input, line)) {
        custom_memdump_.push_back(line);
    }
    once = true;
    return true;
}

void Mem::MemDump() {
    char buf[256];
    char fmt[128];

    if (!ReadMemDump())
        return;

    for(const auto& line : custom_memdump_) {
        char* b = buf;
        const char* ss = line.c_str();
        const char* s = ss;
        while(*s) {
            if (*s == '{') {
                char *end = nullptr;
                uint32_t addr = strtoul(s+1, &end, 16);
                s = end;
                if (*s == ':') {
                    char *f = fmt;
                    *f++ = '%';
                    while(*s != '}') {
                        *f++ = *s++;
                    }
                    *f++ = '\0'; s++;
                    b += sprintf(b, fmt, read_byte_no_io(addr));
                } else if (*s == '}') {
                    b += sprintf(b, "%02x", read_byte_no_io(addr));
                    s++;
                } else {

                    fprintf(stderr, "Unknown delimiter %c\n%s\n%*c\n",
                            *s, ss, int(s-ss), '^');
                    exit(1);
                }
            } else {
                *b++ = *s++;
            }
        }
        *b = '\0';
        ImGui::Text("%s", buf);
    }

}

void Mem::XXXDebugStuff() {
    static bool display_hexdump, display_ppuram, display_memdump;;
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Memory")) {
            ImGui::MenuItem("Hexdump", nullptr, &display_hexdump);
            ImGui::MenuItem("Hexdump PPURam", nullptr, &display_ppuram);
            ImGui::MenuItem("Memdump", nullptr, &display_memdump);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }


    if (display_hexdump) {
        ImGui::Begin("Memory Hexdump", &display_hexdump);
        ImGui::Text("----- NES RAM -----");
        HexDump(0, 2048);
        ImGui::Text("---- Cartridge ----");
        HexDump(0x6000, 0xA000);
        ImGui::End();
    }
    if (display_ppuram) {
        ImGui::Begin("PPU Hexdump", &display_ppuram);
        HexDump(0x2000, 2048);
        ImGui::End();
    }

    if (display_memdump) {
        ImGui::Begin("Custom Memory Dump", &display_memdump);
        MemDump();
        ImGui::End();
    }
}
#endif
}  // namespace protones
