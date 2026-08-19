#include "game/frontend_legacy_globals.h"
#include "game/frontend_state_legacy.h"

void FrontendStateApplyLegacy(const FrontendRuntimeState *state) {
    g_FrameSyncThreshold = state->frameSyncThreshold;
    g_SceneTimer = state->sceneTimer;
    g_FrontendIdleTimer = state->idleTimer;
    g_TitleFadeLevel = state->titleFadeLevel;
    g_MainMenuSlide = state->mainMenuSlide;
    g_TitlePulse = state->titlePulse;
    g_FrontendState = state->frontendState;
    g_TitleExitTimer = state->titleExitTimer;
    g_TitleAttractTimer = state->titleAttractTimer;
}

void FrontendStateCaptureLegacy(FrontendRuntimeState *state) {
    state->frameSyncThreshold = g_FrameSyncThreshold;
    state->sceneTimer = g_SceneTimer;
    state->idleTimer = g_FrontendIdleTimer;
    state->titleFadeLevel = g_TitleFadeLevel;
    state->mainMenuSlide = g_MainMenuSlide;
    state->titlePulse = g_TitlePulse;
    state->frontendState = g_FrontendState;
    state->titleExitTimer = g_TitleExitTimer;
    state->titleAttractTimer = g_TitleAttractTimer;
}
