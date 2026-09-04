#ifndef GAME_ANGLE_INTERNAL_H
#define GAME_ANGLE_INTERNAL_H

#include "common.h"

enum {
    ATAN_TABLE_SAMPLE_COUNT = 1025,
    /* Retail reserves one zero halfword after the addressable samples. */
    ATAN_TABLE_STORAGE_COUNT = ATAN_TABLE_SAMPLE_COUNT + 1,
};

extern s16 g_AtanTable[ATAN_TABLE_STORAGE_COUNT];

#endif
