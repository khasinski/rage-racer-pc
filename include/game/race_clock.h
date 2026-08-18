#ifndef GAME_RACE_CLOCK_H
#define GAME_RACE_CLOCK_H

#include "common.h"

enum RaceClockLimits {
    RACE_CLOCK_MAX_FRAMES = 0x10000,
    RACE_CLOCK_MAX_TIME_MS = 0x927BF
};

typedef struct RaceLapClock {
    s32 frameCount;
    s32 milliseconds;
    s32 saturated;
} RaceLapClock;

s32 RaceClockFramesToMilliseconds(s32 frames, s32 subframeMilliseconds);
RaceLapClock RaceClockTickLap(s32 frameCount, s32 subframeMilliseconds);
s32 RaceClockTickCountdown(s32 remaining, s32 running);
s32 RaceClockSaturateMilliseconds(s32 milliseconds);

#endif
