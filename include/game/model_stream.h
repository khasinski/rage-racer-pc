#ifndef GAME_MODEL_STREAM_H
#define GAME_MODEL_STREAM_H

#include "common.h"

static inline s32 ModelPrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
        return 0x10;
    case 1:
    case 2:
        return 0x18;
    case 3:
        return 0x20;
    default:
        return 0;
    }
}

static inline s32 CoursePrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
        return 0x10;
    case 1:
        return 0x1C;
    case 2:
    case 3:
        return 0x20;
    default:
        return 0;
    }
}

#endif
