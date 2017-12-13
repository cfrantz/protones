#ifndef PROTONES_PBMACRO_H
#define PROTONES_PBMACRO_H
#include "util/macros.h"

#define SAVE_FIELD(pbfield, name) state->set_##pbfield(name);
#define LOAD_FIELD(pbfield, name) name = state->pbfield();

#define SAVE_FIELD1(x) state->set_##x(x##_);
#define LOAD_FIELD1(x) x##_ = state->x();

#define LOAD(...) APPLYX(LOAD_FIELD1, __VA_ARGS__)
#define SAVE(...) APPLYX(SAVE_FIELD1, __VA_ARGS__)

#endif // PROTONES_PBMACRO_H
