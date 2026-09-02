#ifndef GAME_RACE_HUD_INTERNAL_H
#define GAME_RACE_HUD_INTERNAL_H

#include "common.h"

typedef struct StartCountdownTiming {
    s32 visible;
    s32 phase;
    s32 wipeHalfStep;
} StartCountdownTiming;

StartCountdownTiming CalculateStartCountdownTiming(s32 sceneTimer);

#endif
