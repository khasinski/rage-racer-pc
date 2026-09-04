#include "common.h"
#include "game/race_internal.h"

enum {
    MILLISECONDS_PER_SECOND = 1000,
    MILLISECONDS_PER_FRAME =
        MILLISECONDS_PER_SECOND / RACE_FRAMES_PER_SECOND,
};

s32 FramesToMilliseconds(s32 frames, s32 subframeMillis) {
    s32 seconds = frames / RACE_FRAMES_PER_SECOND;
    s32 remainingFrames = frames % RACE_FRAMES_PER_SECOND;
    int64_t milliseconds =
        (int64_t)seconds * MILLISECONDS_PER_SECOND +
        remainingFrames * MILLISECONDS_PER_FRAME + subframeMillis;

    if (milliseconds > INT32_MAX) {
        return INT32_MAX;
    }
    if (milliseconds < INT32_MIN) {
        return INT32_MIN;
    }
    return (s32)milliseconds;
}
