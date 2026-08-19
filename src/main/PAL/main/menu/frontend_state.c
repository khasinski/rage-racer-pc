#include "game/frontend_state.h"
#include "game/frontend_types.h"

enum {
    FRONTEND_DEFAULT_FRAME_SYNC = 0x80
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
    state.titleFadeLevel = returningFromStream ? 0xFF : 0;
    state.titleAttractTimer = returningFromStream ? 0x190 : 0;
    state.titleExitTimer = returningFromStream ? 0 : 0x1E;
    return state;
}

void FrontendStateResetForEntry(FrontendRuntimeState *state) {
    *state = FrontendStateForEntry();
}

void FrontendStateResetForTitle(
    FrontendRuntimeState *state, s32 returningFromStream) {
    *state = FrontendStateForTitle(returningFromStream);
}
