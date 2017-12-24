#ifndef PROTONES_IMWIDGET_CONTROLLER_DEBUG_H
#define PROTONES_IMWIDGET_CONTROLLER_DEBUG_H
#include "imwidget/imwidget.h"

namespace protones {

class NES;
class Controller;

class ControllerDebug : public ImWindowBase {
  public:
    ControllerDebug(NES* nes) : ImWindowBase(false, false), nes_(nes) {}
    bool Draw() override;
  private:
    void DrawController(Controller*);
    NES* nes_;
};

}  // namespace

#endif // PROTONES_IMWIDGET_CONTROLLER_DEBUG_H
