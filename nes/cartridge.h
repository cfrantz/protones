#ifndef PROTONES_NES_CARTRIDGE_H
#define PROTONES_NES_CARTRIDGE_H
#include <string>
#include <cstdint>

#include "nes/base.h"
#include "nes/nes.h"
#include "proto/mappers.pb.h"
namespace protones {

class Cartridge : public EmulatedDevice {
  public:
    struct iNESHeader {
        uint32_t signature;
        uint8_t prgsz;
        uint8_t chrsz;
        uint8_t mirror: 1;
        uint8_t sram: 1;
        uint8_t trainer: 1;
        uint8_t fourscreen: 1;
        uint8_t mapperl: 4;
        uint8_t vs_unisystem: 1;
        uint8_t playchoice_10: 1;
        uint8_t version: 2;
        uint8_t mapperh: 4;
        uint8_t unused[8];
    };
    enum MirrorMode {
        HORIZONTAL,
        VERTICAL,
        SINGLE0,
        SINGLE1,
        FOUR,
    };
    Cartridge(NES* nes);
    ~Cartridge();

    void LoadFile(const std::string& filename);
    void PrintHeader();
    inline uint8_t mirror() const { return mirror_; }
    inline void set_mirror(MirrorMode m) {
        if (!header_.fourscreen) mirror_ = m;
    }
    inline bool battery() const {
        return header_.sram;
    }
    inline uint8_t mapper() const {
        return header_.mapperl | header_.mapperh << 4;
    }
    inline uint32_t prglen() const { return prglen_; }
    inline uint32_t chrlen() const { return chrlen_; }
    inline uint32_t sramlen() const { return sramlen_; }
    inline uint32_t crc32() const { return crc32_; }

    inline uint8_t ReadPrg(uint32_t addr) { return prg_[addr]; }
    inline uint8_t ReadChr(uint32_t addr) { return chr_[addr]; }
    inline uint8_t ReadSram(uint32_t addr) { return sram_[addr]; }
    inline void WritePrg(uint32_t addr, uint8_t val) { prg_[addr] = val; }
    inline void WriteChr(uint32_t addr, uint8_t val) { chr_[addr] = val; }
    inline void WriteSram(uint32_t addr, uint8_t val) { sram_[addr] = val; }
    inline const std::string& filename() { return filename_; }

    void Emulate();
    void SaveSram();

    void LoadState(proto::Mapper* state);
    void SaveState(proto::Mapper* state);
  private:
    NES* nes_;
    struct iNESHeader header_;
    uint8_t *prg_;
    uint32_t prglen_;
    uint8_t *chr_;
    uint32_t chrlen_;
    uint32_t crc32_;
    uint8_t *trainer_;
    MirrorMode mirror_;
    uint8_t *sram_;
    uint32_t sramlen_;
    std::string filename_;
    std::string sram_filename_;
};

}  // namespace protones
#endif // PROTONES_NES_CARTRIDGE_H
