#ifndef GAME_RANDOM_H
#define GAME_RANDOM_H

#include "common.h"

extern u32 g_RandomSeed;

s32 Random15(void);
s32 RandomIndex(s32 count);
s32 RandomRange(s32 minimum, s32 maximum);

#endif
