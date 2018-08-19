#include "imwidget/ppu_debug.h"

#include <cstdint>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "imgui.h"
#include "nes/mapper.h"
#include "nes/mem.h"
#include "nes/nes.h"
#include "nes/ppu.h"

namespace protones {

void PPUTileDebug::TileMemImage(uint32_t* imgbuf, uint16_t addr, int palette,
                       uint8_t* prefcolor) {
    uint32_t pal[] = { 0xFF000000, 0xFF666666, 0xFFAAAAAA, 0xFFFFFFFF };
    uint8_t pcol[4];
    int tile = 0;

    if (palette != -1) {
        for(int c=0; c<4; c++) {
            pal[c] = nes_->palette(nes_->mem()->PaletteRead(palette*4+c));
        }
    }

    for(int y=0; y<16; y++) {
        for(int x=0; x<16; x++, tile++) {
            memset(&pcol, 0, sizeof(pcol));
            for(int row=0; row<8; row++) {
                uint8_t a, b;
                nes_->mapper()->ReadChr2(addr+16*tile+row, &a, &b);
                for(int col=0; col<8; col++, a<<=1, b<<=1) {
                    int color = ((a & 0x80) >> 7) | ((b & 0x80) >> 6);
                    pcol[color]++;
                    imgbuf[128*(8*y + row) + 8*x + col] = pal[color];
                }
            }
            if (pcol[3]>=pcol[1] && pcol[3]>=pcol[2] && pcol[3]>=pcol[0]/3) {
                prefcolor[tile] = 3;
            } else if (pcol[2]>=pcol[1] && pcol[2]>=pcol[0]/3 && pcol[2]>=pcol[3]) {
                prefcolor[tile] = 2;
            } else if (pcol[1]>=pcol[0]/3 && pcol[1]>=pcol[2] && pcol[1]>=pcol[3]) {
                prefcolor[tile] = 1;
            } else {
                prefcolor[tile] = 0;
            }
        }
    }
}

void MakeTexture(GLuint* tid, int x, int y, void* data) {
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, tid);
    glBindTexture(GL_TEXTURE_2D, *tid);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void UpdateTexture(GLuint tid, int x, int y, void* data) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, x, y,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);
}

bool PPUTileDebug::Draw() {
    static uint32_t bank[2][128*128];
    static GLuint bank_tid[2];
    static char palette_names[8][16];
    static char palette_colors[8][4][4];
    static int psel[2];
    static bool once;

    if (!visible_)
        return false;

    if (!once) {
        MakeTexture(&bank_tid[0], 128, 128, bank[0]);
        MakeTexture(&bank_tid[1], 128, 128, bank[1]);
        for(int i=0; i<4; i++) {
            sprintf(palette_names[i],   "Background %d", i);
            sprintf(palette_names[i+4], "    Sprite %d", i);
        }
        once = true;
    }

    ImGui::Begin("Tile Data", &visible_);
    for(int b=0; b<2; b++) {
        ImGui::PushID(b);
        TileMemImage(bank[b], b*0x1000, psel[b], prefcolor_[b]);
        UpdateTexture(bank_tid[b], 128, 128, bank[b]);
        ImGui::BeginGroup();
        ImGui::Text(" ");
        float y = ImGui::GetCursorPosY();
        for(int i=0; i<16; i++) {
            ImGui::SetCursorPosY(y+i*32);
            ImGui::Text("%x0", i);
        }
        ImGui::SetCursorPosY(y);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text(" ");
        float x = ImGui::GetCursorPosX();
        for(int i=0; i<16; i++) {
            ImGui::SameLine(x + i * 32 + 8);
            ImGui::Text("%02x", i);
        }
        ImGui::SetCursorPosY(y);
        ImGui::Image(ImTextureID(uintptr_t(bank_tid[b])), ImVec2(512, 512));
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text(" ");
        ImGui::RadioButton("None", &psel[b], -1);
        for(int p=0; p<8; p++) {
            ImGui::RadioButton(palette_names[p], &psel[b], p);
            for(int c=0; c<4; c++) {
                ImGui::SameLine();
                uint8_t pval = nes_->mem()->PaletteRead(p*4+c);
                ImGui::PushStyleColor(ImGuiCol_Button, nes_->palette(pval));
                sprintf(palette_colors[p][c], "%02x", pval);
                ImGui::Button(palette_colors[p][c]);
                ImGui::PopStyleColor(1);
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::End();
    return false;
}

bool PPUVramDebug::Draw() {
    static const char hex[] = "0123456789abcdef";
    static PPU::Position pos[64][60];
    static PPU::Position ntofs[4] = { {0,0}, {32,0}, {0,30}, {32,30} };
    if (!visible_)
        return false;

    ImGui::Begin("Name Tables", &visible_);
#if 0
    ImGui::Text("ctrl=%02x mask=%02x status=%02x oam=%02x scroll=(%02x %02x)",
        IntVal(&control_),
        IntVal(&mask_),
        IntVal(&status_),
        oam_addr_,
        last_scrollreg_.x,
        last_scrollreg_.y);
#endif

    PPU* ppu = nes_->ppu();
    ImGui::Checkbox("Background", &ppu->debug_showbg_);
    ImGui::SameLine();
    ImGui::Checkbox("Sprites", &ppu->debug_showsprites_);
    ImGui::Text("Video RAM:");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto dump = [=](uint16_t v) {
        char buf[16];
        int xofs = (v & 0x400) ? 32 : 0;
        int yofs = (v & 0x800) ? 30 : 0;
        ImGui::BeginGroup();
        for(int y=0; y<30; y++) {
            for(int x=0; x<32; x++, v++) {
                uint8_t val = nes_->mem()->PPURead(v);

                uint16_t a = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 7);
                uint8_t shift = ((v >> 4) & 4) | (v & 2);
                uint8_t attr = ((nes_->mem()->PPURead(a) >> shift) & 3) << 2;
                uint8_t pval = nes_->mem()->PaletteRead(
                        attr + tile_->prefcolor_[ppu->control_.bgtable][val]);
                if (x == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
                    buf[0] = ' ';
                    buf[1] = hex[(v>>12) & 0xf];
                    buf[2] = hex[(v>>8) & 0xf];
                    buf[3] = hex[(v>>4) & 0xf];
                    buf[4] = hex[(v>>0) & 0xf];
                    buf[5] = ':';
                    buf[6] = 0;
                    ImGui::Text(buf);
#pragma GCC diagnostic pop
                    ImGui::SameLine();
                } else {
                    ImGui::SameLine();
                }
                //pos[xofs+x][yofs+y].x = ImGui::GetCursorPosX();
                //pos[xofs+x][yofs+y].y = ImGui::GetCursorPosY();
                const ImVec2 p = ImGui::GetCursorScreenPos();
                pos[xofs+x][yofs+y].x = p.x;
                pos[xofs+x][yofs+y].y = p.y;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
                //ImGui::TextColored(ImColor(nes_->palette(pval)), "%02x", val);
                buf[0] = hex[(val>>4) & 0xf];
                buf[1] = hex[(val>>0) & 0xf];
                buf[2] = 0;
                ImGui::TextColored(ImColor(nes_->palette(pval)), buf);
#pragma GCC diagnostic pop
            }
        }
        ImGui::EndGroup();
    };

    auto sprites = [=](int x0, int y0, int line, int nto) {
        char buf[8];
        for(int i=0; i<256; i+=4) {
            const PPU::Position *nt = &ntofs[nto];
            int y = ppu->oam_[i + 0];
            if (y != line)
                continue;
            int t = ppu->oam_[i + 1];
            int s = t;
            int a = ppu->oam_[i + 2];
            int x = ppu->oam_[i + 3] + x0;
            int table;
            int ysz;
            if (x > 256) {
                nt = &ntofs[(nto+1) % 4];
                x -= 256;
            }
            if (ppu->control_.spritesize) {
                table = t & 1;
                t &= 0xFE;
                ysz = 32;
            } else {
                table = ppu->control_.spritetable;
                ysz = 16;
            }
            uint8_t pval = nes_->mem()->PaletteRead(
                    0x10 + (a&3)*4 + tile_->prefcolor_[table][t]);
            int xp = pos[nt->x + x/8][nt->y + y/8].x + (x & 7) * 2;
            int yp = pos[nt->x + x/8][nt->y + y/8].y + (y & 7) * 2;
            draw_list->AddRectFilled(ImVec2(xp, yp), ImVec2(xp+16, yp + ysz),
                         ImColor(nes_->palette(pval)));
            buf[0] = hex[(s>>4) & 0xf];
            buf[1] = hex[(s>>0) & 0xf];
            buf[2] = 0;
            draw_list->AddText(ImVec2(xp, yp),
                        ImColor(nes_->palette(pval) ^ 0x00FFFFFF), buf);

            buf[0] = hex[((i/4)>>4) & 0xf];
            buf[1] = hex[((i/4)>>0) & 0xf];
            buf[2] = 0;
            draw_list->AddText(ImVec2(xp, yp+8),
                        ImColor(nes_->palette(pval) ^ 0x00FFFFFF), buf);

        }
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
    dump(0x2000);
    ImGui::SameLine();
    dump(0x2400);

    dump(0x2800);
    ImGui::SameLine();
    dump(0x2c00);
    ImGui::PopStyleVar();

    const ImU32 col32 = ImColor(ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
    struct PPU::Position *nt;
    int lx=0, ly=0, lnt=0;
    for(int i=0; i<240; i++) {
        int x, y, nto, xp, yp;;
        sprites(lx, ly, i, lnt);
        x = ppu->scrollreg_[i].x;
        y = ppu->scrollreg_[i].y;
        nto = ppu->scrollreg_[i].nt;
        if (i && lx==x && ly==y && lnt==nto)
            continue;
        lx = x; ly = y; lnt = nto;
        y += i;
        nt = &ntofs[nto];
        ImGui::Text("%d: x=%d y=%d nt=%d", i, x, y, int(nt-ntofs));
        xp = pos[nt->x + x/8][nt->y + y/8].x + (x & 7) * 2;
        yp = pos[nt->x + x/8][nt->y + y/8].y + (y & 7) * 2;
        ImVec2 ul = ImVec2(xp - 2, yp - 2);
        x += 255; y++;
        for(int j=i+1; j<240; j++, y++) {
            if (ppu->scrollreg_[j].x != lx || ppu->scrollreg_[j].nt != lnt)
                break;
        }
        if (x > 256) {
            nt = &ntofs[(nto + 1) % 4];
            x -= 256;
        }
        xp = pos[nt->x + x/8][nt->y + y/8].x + (x & 7) * 2;
        yp = pos[nt->x + x/8][nt->y + y/8].y + (y & 7) * 2;
        ImVec2 lr = ImVec2(xp + 2, yp + 2);
        int sz = 64;
        draw_list->AddLine(ul, ImVec2(ul.x, ul.y+sz), col32, 2);
        draw_list->AddLine(ul, ImVec2(ul.x+sz, ul.y), col32, 2);
        draw_list->AddLine(ImVec2(lr.x, lr.y-sz), lr, col32, 2);
        draw_list->AddLine(ImVec2(lr.x-sz, lr.y), lr, col32, 2);
    }

    ImGui::End();
    return false;
}

}  // namespace protones
