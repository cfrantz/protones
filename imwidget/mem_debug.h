#ifndef PROTONES_IMWIDGET_MEM_DEBUG_H
#define PROTONES_IMWIDGET_MEM_DEBUG_H
#include "imwidget/imwidget.h"

namespace protones {

class Mem;

class MemDebug : public ImWindowBase {
  public:
    MemDebug(Mem* mem) : ImWindowBase(false, false), mem_(mem) {}
    bool Draw() override;
  private:
    void HexDump(int addr, int len);
    Mem* mem_;
};

}  // namespace

#endif // PROTONES_IMWIDGET_MEM_DEBUG_H
