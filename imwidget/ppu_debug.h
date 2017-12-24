#ifndef PROTONES_IMWIDGET_PPU_DEBUG_H
#define PROTONES_IMWIDGET_PPU_DEBUG_H
#include <cstdint>
#include "imwidget/imwidget.h"

namespace protones {

class NES;

class PPUTileDebug : public ImWindowBase {
  public:
    PPUTileDebug(NES* nes) : ImWindowBase(false, false), nes_(nes) {}
    bool Draw() override;
    void TileMemImage(uint32_t* imgbuf, uint16_t addr, int palette,
                      uint8_t* prefcolor);
  private:
    NES* nes_;
    uint8_t prefcolor_[2][256];
    friend class PPUVramDebug;
};

class PPUVramDebug : public ImWindowBase {
  public:
    PPUVramDebug(NES* nes, const PPUTileDebug* tile)
      : ImWindowBase(false, false),
        nes_(nes),
        tile_(tile) {}
    bool Draw() override;
  private:
    NES* nes_;
    const PPUTileDebug* tile_;
};

}  // namespace

#endif // PROTONES_IMWIDGET_PPU_DEBUG_H
