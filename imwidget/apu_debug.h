#ifndef PROTONES_IMWIDGET_APU_DEBUG_H
#define PROTONES_IMWIDGET_APU_DEBUG_H
#include "nes/base.h"
#include "imwidget/imwidget.h"

namespace protones {

class APU;
class Pulse;
class Triangle;
class Noise;
class DMC;

class APUDebug : public ImWindowBase {
  public:
    APUDebug(APU* apu)
      : ImWindowBase(false, false),
      apu_(apu),
      A4_(440.0) {
          InitFreqTable(A4_);
      }
    bool Draw() override;
  private:
    void DrawPulse(Pulse*);
    void DrawTriangle(Triangle*);
    void DrawNoise(Noise*);
    void DrawDMC(DMC*);
    void DrawOne(APUDevice* dev);

    void InitFreqTable(double a4);
    int FindFreqIndex(double f);

    APU* apu_;
    double A4_;
    double freq_[128];
    static const char* notes_[128];
};

}  // namespace

#endif // PROTONES_IMWIDGET_APU_DEBUG_H
