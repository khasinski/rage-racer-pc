#include "common.h"
#include "game/race_clock.h"

s32 FramesToMilliseconds(s32 frames, s32 millis) {
    return RaceClockFramesToMilliseconds(frames, millis);
}
