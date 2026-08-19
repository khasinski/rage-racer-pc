#ifndef GAME_FRONTEND_STATE_H
#define GAME_FRONTEND_STATE_H

#include "common.h"
#include "game/frontend_types.h"

typedef struct FrontendRuntimeState {
    s32 frameSyncThreshold;
    s32 sceneTimer;
    s32 idleTimer;
    s32 titleFadeLevel;
    s32 mainMenuSlide;
    s32 titlePulse;
    FrontendState frontendState;
    s32 titleExitTimer;
    s32 titleAttractTimer;
} FrontendRuntimeState;

FrontendRuntimeState FrontendStateForEntry(void);
FrontendRuntimeState FrontendStateForTitle(s32 returningFromStream);
void FrontendStateResetForEntry(FrontendRuntimeState *state);
void FrontendStateResetForTitle(
    FrontendRuntimeState *state, s32 returningFromStream);

#endif
