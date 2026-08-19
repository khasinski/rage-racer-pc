#include "game/frontend_state.h"
#include "game/frontend_types.h"

enum {
    FRONTEND_DEFAULT_FRAME_SYNC = 0x80,
    FRONTEND_TITLE_FADE_OPAQUE = 0xFF,
    FRONTEND_TITLE_ATTRACT_DELAY = 0x190,
    FRONTEND_TITLE_ENTRANCE_DELAY = 0x1E
};

FrontendRuntimeState FrontendStateForEntry(void) {
    FrontendRuntimeState state = {0};
    state.frameSyncThreshold = FRONTEND_DEFAULT_FRAME_SYNC;
    state.frontendState = FRONTEND_STATE_TITLE;
    state.titleAttractTimer = -1;
    return state;
}

FrontendRuntimeState FrontendStateForTitle(s32 returningFromStream) {
    FrontendRuntimeState state = {0};
    state.frameSyncThreshold = FRONTEND_DEFAULT_FRAME_SYNC;
    state.frontendState = FRONTEND_STATE_TITLE;
    state.titleFadeLevel = returningFromStream
        ? FRONTEND_TITLE_FADE_OPAQUE : 0;
    state.titleAttractTimer = returningFromStream
        ? FRONTEND_TITLE_ATTRACT_DELAY : 0;
    state.titleExitTimer = returningFromStream
        ? 0 : FRONTEND_TITLE_ENTRANCE_DELAY;
    return state;
}

void FrontendStateResetForEntry(FrontendRuntimeState *state) {
    *state = FrontendStateForEntry();
}

void FrontendStateResetForTitle(
    FrontendRuntimeState *state, s32 returningFromStream) {
    *state = FrontendStateForTitle(returningFromStream);
}
