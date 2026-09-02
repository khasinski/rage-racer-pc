#ifndef GAME_SAVE_TYPES_H
#define GAME_SAVE_TYPES_H

#include "common.h"

typedef struct CourseProgressState {
    u8 bestPlace[4];
    s16 unlockPending;
    s16 retriesRemaining;
} CourseProgressState;

_Static_assert(sizeof(CourseProgressState) == 8,
               "saved course progress size changed");

#endif
