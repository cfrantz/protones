#ifndef PROTONES_PBMACRO_H
#define PROTONES_PBMACRO_H
#include "util/macros.h"

#define SAVE_FIELD(pbfield, name) state->set_##pbfield(name);
#define LOAD_FIELD(pbfield, name) name = state->pbfield();

#define SAVE_FIELD1(x) state->set_##x(x##_);
#define LOAD_FIELD1(x) x##_ = state->x();

#define LOAD(...) APPLYX(LOAD_FIELD1, __VA_ARGS__)
#define SAVE(...) APPLYX(SAVE_FIELD1, __VA_ARGS__)

#define ARRAY_SIZE(a_) (sizeof(a_) / sizeof(a_[0]))

#define LOAD_ARRAY(x) \
    do { \
        size_t i = 0; \
        for(const auto& val : state->x()) { \
            if (i >= ARRAY_SIZE(x##_)) break; \
            x##_[i++] = val; \
        } \
    } while(0);

#define SAVE_ARRAY(x) \
    do { \
        for(const auto& val : x##_) { \
            state->add_##x(val); \
        } \
    } while(0);

#define LOAD_ARRAYS(...) APPLYX(LOAD_ARRAY, __VA_ARGS__)
#define SAVE_ARRAYS(...) APPLYX(SAVE_ARRAY, __VA_ARGS__)

#endif // PROTONES_PBMACRO_H
