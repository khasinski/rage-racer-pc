#include "common.h"

s32 FramesToMilliseconds(s32 frames, s32 millis) {
    s32 seconds = frames / 25;

    frames -= seconds * 25;
    return seconds * 1000 + frames * 40 + millis;
}
