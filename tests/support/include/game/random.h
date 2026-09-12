#ifndef RAGE_TEST_RANDOM_H
#define RAGE_TEST_RANDOM_H

#include "common.h"

extern u32 g_RandomSeed;
s32 Random15(void);

static inline s32 RandomIndex(s32 count) {
    return count > 0 ? (Random15() & 0xFFF) % count : 0;
}

static inline s32 RandomRange(s32 minimum, s32 maximum) {
    int64_t count;

    if (maximum < minimum) {
        return minimum;
    }
    count = (int64_t)maximum - minimum + 1;
    return minimum + (s32)((Random15() & 0xFFF) % count);
}

#endif
