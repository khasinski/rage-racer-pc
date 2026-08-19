#ifndef GAME_RACE_SESSION_LEGACY_H
#define GAME_RACE_SESSION_LEGACY_H

#include "game/race_session_state.h"

typedef enum RaceBootstrapStage {
    RACE_BOOTSTRAP_PLAYER_PREREQUISITES,
    RACE_BOOTSTRAP_TIMING,
    RACE_BOOTSTRAP_LAP_STORAGE,
    RACE_BOOTSTRAP_PRESENTATION,
    RACE_BOOTSTRAP_POST_SCENERY,
    RACE_BOOTSTRAP_CONTROLS,
    RACE_BOOTSTRAP_ACTIVATE
} RaceBootstrapStage;

/* Publishes one lifecycle stage while preserving the retail call ordering. */
void RaceSessionStateApplyLegacyStage(
    const RaceSessionState *state, RaceBootstrapStage stage,
    s32 *retireCameraActive);

#endif
