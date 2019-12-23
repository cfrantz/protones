#include <algorithm>
#include <tuple>
#include "imgui.h"
#include <SDL2/SDL_opengl.h>

#include "nes/pbmacro.h"
#include "nes/cartridge.h"
#include "nes/fm2.h"
#include "nes/ppu.h"
#include "nes/mem.h"
#include "nes/mapper.h"

namespace protones {
namespace {
template<typename T>
uint32_t IntVal(const T* thing) {
    uint32_t val = 0;
    static_assert(sizeof(T) <= sizeof(uint32_t),
            "IntVal is only for small structs");
    memcpy(&val, thing, sizeof(T));
    return val;
}

template<typename T>
void IntVal(T* thing, uint32_t val) {
    static_assert(sizeof(T) <= sizeof(uint32_t),
            "IntVal is only for small structs");
    memcpy(thing, &val, sizeof(T));
}
}

PPU::PPU(NES* nes)
    : nes_(nes),
    cycle_(0), scanline_(0), dead_(1*262*341), frame_(0),
    oam_{0, },
    v_(0), t_(0), x_(0), w_(0), f_(0), register_(0),
    nmi_{0,},
    nametable_(0), attrtable_(0), lowtile_(0), hightile_(0), tiledata_(0),
    sprite_{0,},
    control_{0,},
    mask_{0,},
    status_{0,},
    oam_addr_(0), buffered_data_(0),
    picture_{0,},
    debug_showbg_(true),
    debug_showsprites_(true),
    debug_dot_(0) {
    BuildExpanderTables();
}

void PPU::LoadState(proto::PPU* state) {
    LOAD(cycle, scanline, frame,
         v, t, x, w, f,
         nametable, attrtable, lowtile, hightile, tiledata,
         oam_addr, buffered_data);
    LOAD_FIELD(ppuregister, register_);
    IntVal(&nmi_, state->nmi());
    IntVal(&control_, state->control());
    IntVal(&mask_, state->mask());
    IntVal(&status_, state->status());

    const auto& oam = state->oam();
    memcpy(oam_, oam.data(),
           oam.size() < sizeof(oam_) ? oam.size() : sizeof(oam_));

    const auto& picture = state->picture();
    memcpy(picture_, picture.data(),
           picture.size() < sizeof(picture_) ?
           picture.size() : sizeof(picture_));

    sprite_.count = state->sprite_size();
    for(int i=0; i<sprite_.count; i++) {
        sprite_.pattern[i] = state->sprite(i).pattern();
        sprite_.position[i] = state->sprite(i).position();
        sprite_.priority[i] = state->sprite(i).priority();
        sprite_.index[i] = state->sprite(i).index();
    }
}

void PPU::SaveState(proto::PPU* state) {
    SAVE(cycle, scanline, frame,
         v, t, x, w, f,
         nametable, attrtable, lowtile, hightile, tiledata,
         oam_addr, buffered_data);
    SAVE_FIELD(ppuregister, register_);
    state->set_nmi(IntVal(&nmi_));
    state->set_control(IntVal(&control_));
    state->set_mask(IntVal(&mask_));
    state->set_status(IntVal(&status_));

    auto* oam = state->mutable_oam();
    oam->assign((char*)oam_, sizeof(oam_));
    auto* picture = state->mutable_picture();
    picture->assign((char*)picture_, sizeof(picture_));

    state->clear_sprite();
    for(int i=0; i<sprite_.count; i++) {
        auto* sprite = state->add_sprite();
        sprite->set_pattern(sprite_.pattern[i]);
        sprite->set_position(sprite_.position[i]);
        sprite->set_priority(sprite_.priority[i]);
        sprite->set_index(sprite_.index[i]);
    }
}

void PPU::Reset() {
    dead_ = 2 * (262*341);
    cycle_ = 341-18;;
    scanline_ = 239;
    frame_ = 0;
    set_control(0);
    set_mask(0);
    oam_addr_ = 0;
}

void PPU::NmiChange() {
    uint8_t nmi = nmi_.output & nmi_.occured;
    if (nmi && !nmi_.previous)
        nmi_.delay = 15;
    nmi_.previous = nmi;
}

void PPU::set_control(uint8_t val) {
    control_.nametable =   val & 3;
    control_.increment =   val >> 2;
    control_.spritetable = val >> 3;
    control_.bgtable =     val >> 4;
    control_.spritesize =  val >> 5;
    control_.master =      val >> 6;
    nmi_.output = val >> 7;
    NmiChange();
    t_ = (t_ & 0xF3FF) | (uint16_t(val & 3) << 10);

    if (control_.nametable != last_scrollreg_.nt) {
        last_scrollreg_.nt = control_.nametable;
    }
}

void PPU::set_mask(uint8_t val) {
    uint8_t *mask = (uint8_t*)&mask_;
    *mask = val;
}

uint8_t PPU::status() {
    uint8_t result;
    result = register_ & 0x1F;
    result |= uint8_t(status_.sprite_overflow) << 5;
    result |= uint8_t(status_.sprite0_hit) << 6;
    result |= uint8_t(nmi_.occured) << 7;
    nmi_.occured = false;
    NmiChange();
    w_ = 0;
    return result;
}

void PPU::set_scroll(uint8_t val) {
    if (w_ == 0) {
        t_ = (t_ & 0xFFE0) | (val >> 3);
        x_ = val & 7;
        w_ = 1;

        last_scrollreg_.x = val;
    } else {
        t_ = (t_ & 0x8FFF) | (uint16_t(val & 0x07) << 12);
        t_ = (t_ & 0xFC1F) | (uint16_t(val & 0xF8) << 2);
        w_ = 0;

        last_scrollreg_.y = val;
    }
}

void PPU::set_address(uint8_t val) {
    //printf("set_address w=%d val=%02x ", w_, val);
    if (w_ == 0) {
        t_ = (t_ & 0x80FF) | (uint16_t(val & 0x3F) << 8);
        w_ = 1;
    } else {
        t_ = (t_ & 0xFF00) | val;
        v_ = t_;
        w_ = 0;
    }
    //printf("v_ %04x\n", v_);
}

uint8_t PPU::data() {
    uint8_t result = nes_->mem()->PPURead(v_);

    if (v_ % 0x4000 < 0x3F00) {
        std::swap(buffered_data_, result);
    } else {
        buffered_data_ = nes_->mem()->PPURead(v_ - 0x1000);
    }
    v_ += control_.increment ? 32 : 1;
    return result;
}

void PPU::set_data(uint8_t val) {
    nes_->mem()->PPUWrite(v_, val);
    //printf("set_data: %04x = %02x\n", v_, val);
    v_ += control_.increment ? 32 : 1;
}

void PPU::set_dma(uint8_t val) {
    uint16_t addr = uint16_t(val) << 8;
    for(int i=0; i<256; i++) {
        oam_[oam_addr_++] = nes_->mem()->Read(addr++);
    }
    // TODO(cfrantz):
    // stall cpu for 513 + (cpu.cycles % 2) cycles.
    nes_->Stall(513 + nes_->cpu_cycles() % 2);
}

uint8_t PPU::Read(uint16_t addr) {
    switch(addr) {
        case 0x2002: return status();
        case 0x2004: return oam_[oam_addr_];
        case 0x2007: return data();
    }
    return 0;
}

void PPU::Write(uint16_t addr, uint8_t val) {
    register_ = val;
    switch(addr) {
        case 0x2000: set_control(val); break;
        case 0x2001: set_mask(val); break;
        case 0x2003: oam_addr_ = val; break;
        case 0x2004: oam_[oam_addr_++] = val; break;
        case 0x2005: set_scroll(val); break;
        case 0x2006: set_address(val); break;
        case 0x2007: set_data(val); break;
        case 0x4014: set_dma(val); break;
    }
}

void PPU::IncrementX() {
    if ((v_ & 0x001f) == 0x1f) {
        v_ = (v_ & 0xFFE0) ^ 0x0400;
    } else {
        v_++;
    }
}

void PPU::IncrementY() {
    if ((v_ & 0x7000) != 0x7000) {
        v_ += 0x1000;
    } else {
        v_ = v_ & 0x8FFF;
        uint16_t y = (v_ & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            v_ = v_ ^ 0x800;
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        v_ = (v_ & 0xFC1F) | (y << 5);
    }
}

void PPU::CopyX() {
    v_ = (v_ & 0xFBE0) | (t_ & 0x041F);
}

void PPU::CopyY() {
    v_ = (v_ & 0x841F) | (t_ & 0x7BE0);
}

void PPU::SetVerticalBlank() {
    nmi_.occured = true;
    NmiChange();
    //nes_->io()->screen_blit(picture_);
}

void PPU::ClearVerticalBlank() {
    nmi_.occured = false;
    NmiChange();
}

void PPU::FetchNameTableByte() {
    nametable_ = nes_->mem()->PPURead(0x2000 | (v_ & 0x0FFF));
}

void PPU::FetchAttributeByte() {
    uint16_t a = 0x23C0 | (v_ & 0x0C00) | ((v_ >> 4) & 0x38) | ((v_ >> 2) & 7);
    uint8_t shift = ((v_ >> 4) & 4) | (v_ & 2);
    attrtable_ = ((nes_->mem()->PPURead(a) >> shift) & 3) << 2;
}

void PPU::FetchLowTileByte() {
    uint16_t a = (0x1000 * control_.bgtable) + (16 * nametable_) +
                 ((v_ >> 12) & 7);
    // Fetch both the low and high bytes in one call
    nes_->mapper()->ReadChr2(a, &lowtile_, &hightile_);
}

void PPU::FetchHighTileByte() {
    // Used to fetch the high byte here, but now nothing
}

void PPU::BuildExpanderTables() {
    // Precompute expanded 8-bit patterns into 32-bits to we can build the
    // pattern+attribute words later without any loops.
    // We compute both the normal and reflected value:
    // 8 bit abcdefgh -> 000a000b000c000d000e000f000g000h
    //                -> 000h000g000f000e000d000c000b000a
    uint8_t val;
    for(int n=0; n<256; n++) {
        uint32_t data = 0;
        val = n;
        for(int bit=0; bit<8; bit++) {
            data = (data<<4) | (val & 0x80)>>7;
            val <<= 1;
        }
        reflection_table_[n] = data;

        val = n;
        for(int bit=0; bit<8; bit++) {
            data = (data<<4) | (val & 1);
            val >>= 1;
        }
        normal_table_[n] = data;
    }
}

void PPU::StoreTileData() {
    uint64_t data = 0;
    // Expand the 2-bit attribute value into every nybble of the word
    // so we can just or it with the tile pattern data.
    uint32_t aa = 0x11111111 * uint32_t(attrtable_);
    data = reflection_table_[lowtile_] | reflection_table_[hightile_]<<1;
    tiledata_ |= (data | aa);
}

uint8_t PPU::BackgroundPixel() {
    if (!mask_.showbg)
        return 0;
    uint32_t data = (tiledata_ >> 32) >> ((7-x_) * 4);
    return uint8_t(data & 0x0F);
}

uint16_t PPU::SpritePixel() {
    if (mask_.showsprites) {
        for(int i=0; i < sprite_.count; i++) {
            uint32_t offset = cycle_ - 1 - sprite_.position[i];
            if (offset > 7)
                continue;
            offset = 7 - offset;
            uint8_t color = (sprite_.pattern[i] >> (offset*4)) & 0x0F;
            if (color % 4 == 0)
                continue;

            return (i<<8) | color;
        }
    }
    return 0;
}


void PPU::RenderPixel() {
    int x = cycle_ - 1;
    int y = scanline_;
    uint8_t background = BackgroundPixel();
    auto sp = SpritePixel();
    uint8_t i = sp>>8, sprite = sp & 0xff;
    uint8_t color;

    if (x < 8) {
        if (!mask_.showleftbg) background = 0;
        if (!mask_.showleftsprite) sprite = 0;
    }

    bool b = (background % 4) != 0;
    bool s = (sprite % 4) != 0;

    if (!b) {
        color = debug_showsprites_ ?
            (s ? (sprite | 0x10) : 0) : 0;
    } else if (!s) {
        color = debug_showbg_ ? background : 0;
    } else {
        if (sprite_.index[i] == 0 && x < 255)
            status_.sprite0_hit = 1;

        if (sprite_.priority[i] == 0) {
            color = debug_showsprites_ ? (sprite | 0x10) : 0;
        } else {
            color = debug_showbg_ ? background : 0;
        }
    }
    if (debug_dot_) {
        picture_[y * 256 + x] = debug_dot_;
        debug_dot_ = 0;
    } else {
        picture_[y * 256 + x] = nes_->palette(nes_->mem()->PaletteRead(color));
    }
}

uint32_t PPU::FetchSpritePattern(int i, int row) {
    uint8_t tile = oam_[i*4 + 1];
    uint8_t attr = oam_[i*4 + 2];
    uint16_t addr;
    uint16_t table;

    if (!control_.spritesize) {
        if (attr & 0x80)
            row = 7-row;
        table = control_.spritetable;
    } else {
        if (attr & 0x80)
            row = 15-row;
        table = tile & 1;
        tile = tile & 0xFE;
        if (row > 7) {
            tile++; row -= 8;
        }
    }

    addr = 0x1000 * table + tile * 16 + row;
    uint8_t a = (attr & 3) << 2;
    uint8_t lo, hi;
    nes_->mapper()->ReadSpr2(addr, &lo, &hi);
    uint32_t result = 0x11111111 * uint32_t(a);

    if (attr & 0x40) {
        result |= normal_table_[lo] | normal_table_[hi]<<1;
    } else {
        result |= reflection_table_[lo] | reflection_table_[hi]<<1;
    }
    return result;
}

void PPU::EvaluateSprites() {
    int h = (control_.spritesize) ? 16 : 8;
    int count = 0;

    for(int i=0; i<64; i++) {
        uint8_t y = oam_[i*4 + 0];
        uint8_t a = oam_[i*4 + 2];
        uint8_t x = oam_[i*4 + 3];
        int row = scanline_ - y;

        if (!(row >= 0 && row < h))
            continue;

        if (count < 8) {
            sprite_.pattern[count] = FetchSpritePattern(i, row);
            sprite_.position[count] = x;
            sprite_.priority[count] = (a >> 5) & 1;
            sprite_.index[count] = i;
        }
        ++count;
    }
    if (count > 8) {
        count = 8;
        status_.sprite_overflow = 1;
    }
    sprite_.count = count;
}

bool PPU::Tick() {
    if (dead_) {
        --dead_;
        return false;
    }
    if (nmi_.delay) {
        nmi_.delay--;
        if (nmi_.delay == 0 && nmi_.output && nmi_.occured)
            nes_->NMI();
    }

    if (mask_.showbg || mask_.showsprites) {
        if (f_ == 1 && scanline_ == 261 && cycle_ == 339) {
            cycle_ = 0;
            scanline_ = 0;
            frame_++;
            f_ = f_ ^ 1;
            return true;
        }
    }

    cycle_++;
    if (cycle_ > 340) {
        cycle_ = 0;
        scanline_++;
        if (scanline_ > 261) {
            scanline_ = 0;
            frame_++;
            f_ = f_ ^ 1;
        }
    }
    return true;
}

void PPU::Emulate() {
    if (!Tick()) {
        return;
    }

    const bool pre_line = scanline_ == 261;
    const bool visible_line = scanline_ < 240;
    const bool render_line = pre_line || visible_line;
    const bool prefetch_cycle = (cycle_ >= 321 && cycle_ <= 336);
    const bool visible_cycle = (cycle_ > 0 && cycle_ <= 256);
    const bool fetch_cycle = prefetch_cycle || visible_cycle;

    if (visible_line && visible_cycle)
        RenderPixel();

    if (mask_.showbg || mask_.showsprites) {
        if (render_line && fetch_cycle) {
            tiledata_ <<= 4;
            switch(cycle_ % 8) {
            case 0: StoreTileData(); break;
            case 1: FetchNameTableByte(); break;
            case 3: FetchAttributeByte(); break;
            case 5: FetchLowTileByte(); break;
            case 7: FetchHighTileByte(); break;
            }
        }

        if (pre_line && cycle_ >= 280 && cycle_ <= 304)
            CopyY();

        if (render_line) {
            if (fetch_cycle && (cycle_ % 8) == 0)
                IncrementX();
            if (cycle_ == 256)
                IncrementY();
            if (cycle_ == 257)
                CopyX();
        }
        if (cycle_ == 257) {
            if (visible_line) {
                EvaluateSprites();
            } else {
                sprite_.count = 0;
            }
        }
    }

    if (scanline_ == 241 && cycle_ == 1) {
        SetVerticalBlank();
    }
    if (pre_line && cycle_ == 1) {
        ClearVerticalBlank();
        status_.sprite0_hit = 0;
        status_.sprite_overflow = 0;
    }
    if (cycle_ == 1) {
        scrollreg_[scanline_].x = last_scrollreg_.x;
        scrollreg_[scanline_].y = last_scrollreg_.y;
        scrollreg_[scanline_].nt = last_scrollreg_.nt;
    }

}
}  // namespace protones
