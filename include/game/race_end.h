#ifndef GAME_RACE_END_H
#define GAME_RACE_END_H

#include "common.h"

typedef struct RaceEndState {
    s16 phase;
    s16 grandPrixMode;
    s32 fadeTimer;
    s32 retriesRemaining;
} RaceEndState;

typedef struct RaceEndCommands {
    s32 drawEndBannerIntensity;
    s32 drawLostCaptionIntensity;
    s32 drawFadeIntensity;
    s32 requestCdTrack;
    s32 startCd;
    s32 exitScene;
    s32 disableMirror;
} RaceEndCommands;

void RaceEndStep(RaceEndState *state, RaceEndCommands *commands);

#endif
