#ifndef GAME_SAVE_TYPES_H
#define GAME_SAVE_TYPES_H

#include "common.h"

typedef struct CourseProgressState {
    u8 bestPlace[4];
    s16 unlockPending;
    s16 retriesRemaining;
} CourseProgressState;

#endif
