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

typedef struct RaceTiming {
    s32 lapTime;
    s32 bestLap;
    s32 sectorEnds[3];
    s32 sectorIndex;
    s32 sectorTimes[3];
    s32 refLapTime;
    SectorReferenceTimes refSectorTimes;
    s32 lastSectorTime;
    s32 splitDelta;
    s32 splitTargetTime;
    s16 splitSign;
    s16 splitSector;
    s16 splitTimer;
} RaceTiming;

#endif
