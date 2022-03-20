#ifndef PROTONES_NES_VRC7_AUDIO_VRC7_AUDIO_H
#define PROTONES_NES_VRC7_AUDIO_VRC7_AUDIO_H

#include <stdint.h>

#if defined(VRC7_AUDIO_EMU2413)
#include "nes/vrc7_audio/emu2413.h"
#define VRC7_AUDIO_NCHAN 6
typedef OPLL* VRC7AudioPtr;

VRC7AudioPtr VRC7Audio_New(uint32_t clock, uint32_t rate) {
    return OPLL_new(clock, rate);
}

void VRC7Audio_Delete(VRC7AudioPtr p) {
    OPLL_delete(p);
}

void VRC7Audio_WriteReg(VRC7AudioPtr p, uint32_t reg, uint32_t val) {
    OPLL_writeReg(p, reg, val);
}

void VRC7Audio_Output(VRC7AudioPtr p, float output[VRC7_AUDIO_NCHAN]) {
    OPLL_output(p, output);
}

#elif defined(VRC7_AUDIO_DSA_EMU2413)
#include "nes/vrc7_audio/dsa_emu2413.h"
#define VRC7_AUDIO_NCHAN 10
typedef OPLL* VRC7AudioPtr;

VRC7AudioPtr VRC7Audio_New(uint32_t clock, uint32_t rate) {
    VRC7AudioPtr p = OPLL_new(clock, rate);
    // For VRC7, the chip type is 1.
    OPLL_setChipType(p, 1);
    OPLL_resetPatch(p, 1);
    return p;
}

void VRC7Audio_Delete(VRC7AudioPtr p) {
    OPLL_delete(p);
}

void VRC7Audio_WriteReg(VRC7AudioPtr p, uint32_t reg, uint32_t val) {
    OPLL_writeReg(p, reg, val);
}

void VRC7Audio_Output(VRC7AudioPtr p, float output[VRC7_AUDIO_NCHAN]) {
    OPLL_calc(p);
    for(int i=0; i<9; i++) {
        output[i] = float(p->ch_out[i]) / 2048.0;
    }
    output[9] = 0;
    for(int i=9; i<14; i++) {
        output[9] += float(p->ch_out[i]) / 2048.0;
    }
}

#elif defined(VRC7_AUDIO_YM2413)
extern "C" {
#include "nes/vrc7_audio/ym2413.h"
}
#define VRC7_AUDIO_NCHAN 10
typedef uint8_t* VRC7AudioPtr;

VRC7AudioPtr VRC7Audio_New(uint32_t clock, uint32_t rate) {
    YM2413Init();
    YM2413ResetChip();
    return nullptr;
}

void VRC7Audio_Delete(VRC7AudioPtr p) {
}

void VRC7Audio_WriteReg(VRC7AudioPtr p, uint32_t reg, uint32_t val) {
    YM2413Write(0, reg);
    YM2413Write(1, val);
}

void VRC7Audio_Output(VRC7AudioPtr p, float output[VRC7_AUDIO_NCHAN]) {
    YM2413Output(output);
}

#else
#error "No VRC7 Audio Implementation"
#endif

#endif // PROTONES_NES_VRC7_AUDIO_VRC7_AUDIO_H
