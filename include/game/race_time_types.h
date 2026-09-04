#ifndef GAME_RACE_TIME_TYPES_H
#define GAME_RACE_TIME_TYPES_H

#include "common.h"

typedef union SectorReferenceTimes {
    s32 values[3];
    struct {
        s32 first;
        s32 second;
        s32 third;
    } fields;
} SectorReferenceTimes;

_Static_assert(sizeof(SectorReferenceTimes) == 3 * sizeof(s32),
               "sector reference time layout changed");

extern SectorReferenceTimes g_RefSectorTimes;

#endif
