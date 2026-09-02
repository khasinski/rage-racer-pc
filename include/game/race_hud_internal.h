#ifndef GAME_RACE_HUD_INTERNAL_H
#define GAME_RACE_HUD_INTERNAL_H

#include "common.h"

typedef struct StartCountdownTiming {
    s32 visible;
    s32 phase;
    s32 wipeHalfStep;
} StartCountdownTiming;

StartCountdownTiming CalculateStartCountdownTiming(s32 sceneTimer);

typedef struct RaceOptionMarqueeState {
    s32 firstScroll;
    s32 secondScroll;
    s32 brightness;
    s32 textOffset;
} RaceOptionMarqueeState;

RaceOptionMarqueeState AdvanceRaceOptionMarquee(s32 firstScroll,
                                                s32 secondScroll,
                                                s32 sceneTimer);

typedef struct RaceOptionPulseState {
    s32 angle;
    s32 halfWidth;
} RaceOptionPulseState;

RaceOptionPulseState AdvanceRaceOptionPulse(s32 angle);

#endif
