#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <gflags/gflags.h>

#include "nes/cartridge.h"
#include "util/crc.h"

DEFINE_bool(sram_on_disk, true, "Save SRAM to disk.");

namespace protones {

Cartridge::Cartridge(NES* nes)
    : nes_(nes),
    prg_(nullptr), prglen_(0),
    chr_(nullptr), chrlen_(0),
    crc32_(0),
    trainer_(nullptr),
    sram_{0,} {
}


Cartridge::~Cartridge() {
    delete[] prg_;
    delete[] chr_;
    delete[] trainer_;
}


void Cartridge::LoadFile(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "rb");

    if (fp == nullptr) {
        fprintf(stderr, "Couldn't read %s.\n", filename.c_str());
        abort();
    }
    if (fread(&header_, sizeof(header_), 1, fp) != 1) {
        fprintf(stderr, "Couldn't read header.\n");
        abort();
    }

    mirror_ = MirrorMode(
            header_.fourscreen ? MirrorMode::FOUR : header_.mirror);
    prglen_ = 16384 * header_.prgsz;
    prg_ = new uint8_t[prglen_];

    if (header_.chrsz) {
        chrlen_ = 8192 * header_.chrsz;
    } else {
        // No chr rom, need and 8k buffer (ram?)
        chrlen_ = 8192;
    }
    chr_ = new uint8_t[chrlen_];

    if (header_.trainer) {
        trainer_ = new uint8_t[512];
        if (fread(trainer_, 1, 512, fp) != 512) {
            fprintf(stderr, "Couldn't read trainer.\n");
            abort();
        }
    }

    if (fread(prg_, 16384, header_.prgsz, fp) != header_.prgsz) {
        fprintf(stderr, "Couldn't read PRG.\n");
        abort();
    }

    if (fread(chr_, 8192, header_.chrsz, fp) != header_.chrsz) {
        fprintf(stderr, "Couldn't read CHR.\n");
        abort();
    }
    fclose(fp);
    crc32_ = Crc32(0, prg_, prglen_);
    crc32_ = Crc32(crc32_, chr_, chrlen_);

    sram_filename_ = filename + ".sram";
    if (FLAGS_sram_on_disk && header_.sram && !nes_->has_movie()) {
        if ((fp = fopen(sram_filename_.c_str(), "rb")) != nullptr) {;
            if (fread(sram_, sizeof(sram_), 1, fp) == 0) {
                fprintf(stderr, "Couldn't read SRAM.\n");
            }
            fclose(fp);
        }
    }
    PrintHeader();
}

void Cartridge::Emulate() {
    static uint64_t save_frame;

    if (nes_->frame() - save_frame >= 60) {
        save_frame = nes_->frame();
        SaveSram();
    }
}

void Cartridge::SaveSram() {
    if (!FLAGS_sram_on_disk)
        return;
    if (!header_.sram)
        return;

    FILE *fp = fopen(sram_filename_.c_str(), "wb");
    if (!fp) {
        fprintf(stderr, "Can't open %s for writing.\n", sram_filename_.c_str());
        return;
    }
    fwrite(sram_, 1, sizeof(sram_), fp);
    fclose(fp);
}

void Cartridge::SaveState(proto::Mapper *state) {
    auto* wram = state->mutable_wram();
    wram->assign((char*)sram_, sizeof(sram_));
}

void Cartridge::LoadState(proto::Mapper *state) {
    const auto& wram = state->wram();
    memcpy(sram_, wram.data(),
           wram.size() < sizeof(sram_) ? wram.size() : sizeof(sram_));
}

void Cartridge::PrintHeader() {
    uint8_t *bytes = (uint8_t*)&header_;
    printf("NES header:\n");
    printf("  %02x %02x %02x %02x %02x %02x %02x %02x\n",
           bytes[0], bytes[1], bytes[2], bytes[3],
           bytes[4], bytes[5], bytes[6], bytes[7]);
    bytes += 8;
    printf("  %02x %02x %02x %02x %02x %02x %02x %02x\n",
           bytes[0], bytes[1], bytes[2], bytes[3],
           bytes[4], bytes[5], bytes[6], bytes[7]);

    printf("  PRG banks: %d\n", header_.prgsz);
    printf("  CHR banks: %d\n", header_.chrsz);
    printf("  Mirroring: %d\n", mirror_);
    printf("  Has SRAM:  %d\n", header_.sram);
    printf("  Has trainer: %d\n", header_.trainer);
    printf("  Mapper: %d\n", mapper());
    printf("  ROM CRC32: 0x%08x\n", crc32_);
}
}  // namespace protones
