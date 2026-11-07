#ifndef GAME_RANDOM_H
#define GAME_RANDOM_H

#include "common.h"

extern u32 g_RandomSeed;

/* Explicit state for independent races and reproducible simulation steps. */
static inline s32 RandomNext(u32 *state) {
    *state = *state * 0x41C64E6Du + 0x3039u;
    return (s32)((*state >> 16) & 0x7FFFu);
}

s32 Random15(void);
s32 RandomIndex(s32 count);
s32 RandomRange(s32 minimum, s32 maximum);

#endif
