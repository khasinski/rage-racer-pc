#include "game/random.h"

enum {
    RANDOM_MULTIPLIER = 0x41C64E6D,
    RANDOM_INCREMENT = 0x3039,
    RANDOM_RESULT_MASK = 0x7FFF,
    RANDOM_SELECTION_MASK = 0xFFF,
};

s32 Random15(void) {
    u32 seed = g_RandomSeed * RANDOM_MULTIPLIER + RANDOM_INCREMENT;
    s32 result = (seed >> 16) & RANDOM_RESULT_MASK;

    g_RandomSeed = seed;
    return result;
}

s32 RandomIndex(s32 count) {
    return count > 0 ? (Random15() & RANDOM_SELECTION_MASK) % count : 0;
}

s32 RandomRange(s32 minimum, s32 maximum) {
    int64_t count;

    if (maximum < minimum) {
        return minimum;
    }
    count = (int64_t)maximum - minimum + 1;
    return minimum +
           (s32)((Random15() & RANDOM_SELECTION_MASK) % count);
}
