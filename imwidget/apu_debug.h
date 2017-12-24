#ifndef PROTONES_IMWIDGET_APU_DEBUG_H
#define PROTONES_IMWIDGET_APU_DEBUG_H
#include "imwidget/imwidget.h"

namespace protones {

class APU;
class Pulse;
class Triangle;
class Noise;
class DMC;

class APUDebug : public ImWindowBase {
  public:
    APUDebug(APU* apu) : ImWindowBase(false, false), apu_(apu) {}
    bool Draw() override;
  private:
    void DrawPulse(Pulse*);
    void DrawTriangle(Triangle*);
    void DrawNoise(Noise*);
    void DrawDMC(DMC*);
    APU* apu_;
};

}  // namespace

#endif // PROTONES_IMWIDGET_APU_DEBUG_H
