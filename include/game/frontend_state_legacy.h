#ifndef GAME_FRONTEND_STATE_LEGACY_H
#define GAME_FRONTEND_STATE_LEGACY_H

#include "game/frontend_state.h"

void FrontendStateApplyLegacy(const FrontendRuntimeState *state);
void FrontendStateCaptureLegacy(FrontendRuntimeState *state);

#endif
