#include "game/random.h"

enum {
    RANDOM_SELECTION_MASK = 0xFFF,
};

s32 Random15(void) {
    return RandomNext(&g_RandomSeed);
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
