#include "game/random.h"

enum {
    RANDOM_MULTIPLIER = 0x41C64E6D,
    RANDOM_INCREMENT = 0x3039,
    RANDOM_RESULT_MASK = 0x7FFF,
};

s32 Random15(void) {
    u32 seed = g_RandomSeed * RANDOM_MULTIPLIER + RANDOM_INCREMENT;
    s32 result = (seed >> 16) & RANDOM_RESULT_MASK;

    g_RandomSeed = seed;
    return result;
}
