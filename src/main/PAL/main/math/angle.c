#include "game/render.h"

#include <stdint.h>

enum {
    ATAN_TABLE_SAMPLE_COUNT = 1025,
};

static const s16 s_atanTable[ATAN_TABLE_SAMPLE_COUNT] = {
#include "atan_table.inc"
};

static s32 FirstQuadrantAngle(uint64_t x, uint64_t y) {
    uint64_t tableIndex;

    if (x < y) {
        tableIndex = (x << 10) / y;
        return 0x400 - s_atanTable[tableIndex];
    }
    tableIndex = (y << 10) / x;
    return s_atanTable[tableIndex];
}

s32 Atan2(s32 x, s32 y) {
    uint64_t magnitudeX;
    uint64_t magnitudeY;
    s32 angle;

    if (x == 0) {
        if (y == 0) return 0;
        return y > 0 ? 0x400 : -0x400;
    }

    magnitudeX = x < 0 ? (uint64_t)-(int64_t)x : (uint64_t)x;
    magnitudeY = y < 0 ? (uint64_t)-(int64_t)y : (uint64_t)y;
    angle = FirstQuadrantAngle(magnitudeX, magnitudeY);

    if (x > 0) return y >= 0 ? angle : -angle;
    return y >= 0 ? 0x800 - angle : 0x800 + angle;
}
