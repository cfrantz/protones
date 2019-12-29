#ifndef PROTONES_NES_BASE_H
#define PROTONES_NES_BASE_H
#include <cstring>

namespace protones {
class EmulatedDevice {
  public:
    ~EmulatedDevice() {}
};

class APUDevice {
  public:
    APUDevice(const char* name, float volume)
      : output_volume_(volume),
      dbgbuf_{0},
      dbgp_(0)
    {
        set_name(name);
    }

    ~APUDevice() {}
    enum class Type {
        Unknown,
        Pulse,
        Triangle,
        Noise,
        DMC,
    };

    virtual Type type() const = 0;
    const char* name() const { return name_; }
    void set_name(const char* name) {
        if (name) {
            strncpy(name_, name, sizeof(name_)-1);
            name_[sizeof(name_)-1] = 0;
        }
    }
    float* mutable_output_volume() { return &output_volume_; }

  protected:
    char name_[64];
    float output_volume_;
    constexpr static int DBGBUFSZ = 1024;
    float dbgbuf_[DBGBUFSZ];
    int dbgp_;
    friend class APUDebug;
};

}  // namespace protones
#endif // PROTONES_NES_BASE_H
