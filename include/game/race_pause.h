#ifndef GAME_RACE_PAUSE_H
#define GAME_RACE_PAUSE_H

#include "common.h"

typedef struct RacePauseState {
    s32 sceneTimer;
    s32 paused;
    s32 debounce;
    s16 phase;
    s16 optionCursor;
    s16 grandPrixMode;
    s16 fadeTimer;
    s32 retriesRemaining;
    s32 retireCameraActive;
} RacePauseState;

typedef struct RacePauseCommands {
    s32 pauseCd;
    s32 resumeCd;
    s32 effectVoicesEnabled;
    s32 setEffectVoices;
    s32 setPauseReverb;
    s32 seedFinishCamera;
    s32 startCdFadeFrames;
    s32 exitRaceScene;
    s32 updateTimeAttackRecord;
    s32 soundCues[2];
    s32 soundCueCount;
} RacePauseCommands;

void RacePauseStep(RacePauseState *state, u16 pressed,
                   RacePauseCommands *commands);

#endif
