#include "common.h"

enum {
    GAME_FRAMES_PER_SECOND = 25,
    MILLISECONDS_PER_SECOND = 1000,
    MILLISECONDS_PER_FRAME =
        MILLISECONDS_PER_SECOND / GAME_FRAMES_PER_SECOND,
};

s32 FramesToMilliseconds(s32 frames, s32 subframeMillis) {
    s32 seconds = frames / GAME_FRAMES_PER_SECOND;
    s32 remainingFrames = frames % GAME_FRAMES_PER_SECOND;

    return seconds * MILLISECONDS_PER_SECOND +
           remainingFrames * MILLISECONDS_PER_FRAME + subframeMillis;
}
