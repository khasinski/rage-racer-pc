#include "common.h"
#include "game/render.h"


/*
 * PSY-Q 3.5 libgte object geo_00.o (LIBGTE.A): the fixed-point sine/cosine
 * helpers rsin, rsinCore and rcos
 * (rcos) that read the 0x80094308 sine table.  Byte-matched against
 * geo_00.o (rsin anchor).
 */

s32 rsin(s32 angle) {
    if (angle < 0) {
        return -rsinCore(-angle & 0xFFF);
    }

    return rsinCore(angle & 0xFFF);
}

s32 rsinCore(s32 angle) {
    if (angle < 0x801) {
        if (angle < 0x401) {
            return g_SinTable[angle];
        }

        return g_SinTable[0x800 - angle];
    }

    if (angle < 0xC01) {
        return -D_80093308[angle];
    }

    return -g_SinTable[0x1000 - angle];
}

s32 rcos(s32 angle) {
    if (angle < 0) {
        angle = -angle;
    }

    angle &= 0xFFF;

    if (angle < 0x801) {
        if (angle < 0x401) {
            return g_SinTable[0x400 - angle];
        }

        return -D_80093B08[angle];
    }

    if (angle < 0xC01) {
        return -g_SinTable[0xC00 - angle];
    }

    return D_80092B08[angle];
}
