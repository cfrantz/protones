#ifndef PROTONES_NES_FM2_H
#define PROTONES_NES_FM2_H
#include <string>
#include "nes/base.h"
#include "nes/nes.h"
namespace protones {

class FM2Movie : public EmulatedDevice {
  public:
    FM2Movie(NES* nes);
    void Load(const std::string& filename);
    void Emulate();
  private:
    void Parse(const std::string& s);
    NES* nes_;
    bool loaded_;

};

}  // namespace protones
#endif // PROTONES_NES_FM2_H
