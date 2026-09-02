#ifndef GAME_RACE_HUD_INTERNAL_H
#define GAME_RACE_HUD_INTERNAL_H

#include "common.h"

typedef struct StartCountdownTiming {
    s32 visible;
    s32 phase;
    s32 wipeHalfStep;
} StartCountdownTiming;

StartCountdownTiming CalculateStartCountdownTiming(s32 sceneTimer);

typedef struct StartCountdownRow {
    u32 pattern;
    s32 colorBank;
} StartCountdownRow;

StartCountdownRow BuildStartCountdownRow(s32 phase, s32 row,
                                         s32 wipeHalfStep,
                                         const u32 *glyphPatterns,
                                         const u32 *firstPattern);
s32 AdvanceStartCountdownBoard(s32 phase, s32 currentOffset);

typedef struct StartCountdownLamp {
    s32 intensity;
    u16 clut;
} StartCountdownLamp;

StartCountdownLamp BuildStartCountdownLamp(s32 phase, s32 sceneTimer,
                                           s32 lampIndex);

typedef struct RaceOptionMarqueeState {
    s32 firstScroll;
    s32 secondScroll;
    s32 brightness;
    s32 textFrame;
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
