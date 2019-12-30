#include "imwidget/mem_debug.h"

#include "imgui.h"
#include "nes/mem.h"
#include "nes/cartridge.h"

namespace protones {

void MemDebug::HexDump(int addr, int len) {
    static const char hex[] = "0123456789abcdef";
    char line[80];
    char *b = line;
    int i, c=55;
    uint8_t val;

    for(i=0; i < len; i++) {
        uint16_t a = addr + i;
        val = mem_->read_byte_no_io(a);
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

bool MemDebug::Draw() {
    if (!visible_)
        return false;

    ImGui::Begin("Memory", &visible_);
    ImGui::Text("----- NES RAM -----");
    HexDump(0, 2048);
    if (mem_->nes_->cartridge()->mapper() == 5) {
        ImGui::Text("---- MMC5 ExtRAM ----");
        HexDump(0x5c00, 1024);
    }
    ImGui::Text("---- Cartridge ----");
    HexDump(0x6000, 0xA000);
    ImGui::End();
    return false;
}

}  // namespace protones
