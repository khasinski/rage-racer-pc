#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/race_session_legacy.h"
#include "game/state.h"
#include "game/track.h"

void RaceSessionStateApplyLegacyStage(
    const RaceSessionState *state, RaceBootstrapStage stage,
    s32 *retireCameraActive) {
    switch (stage) {
    case RACE_BOOTSTRAP_PLAYER_PREREQUISITES:
        g_LapCount = state->lapCount;
        break;
    case RACE_BOOTSTRAP_TIMING:
        g_LapCount = state->lapCount;
        g_LapTimeMs = state->lapTimeMs;
        g_LapTimeSaturated = state->lapTimeSaturated;
        g_SectorEndDistance[0] = state->sectorEndDistance[0];
        g_SectorEndDistance[1] = state->sectorEndDistance[1];
        g_SectorEndDistance[2] = state->sectorEndDistance[2];
        g_SectorIndex = state->sectorIndex;
        g_RaceTimeRemaining = state->raceTimeRemaining;
        break;
    case RACE_BOOTSTRAP_LAP_STORAGE:
        g_RaceTotalTime = state->raceTotalTime;
        break;
    case RACE_BOOTSTRAP_PRESENTATION:
        g_AnimTimer = state->animTimer;
        g_SceneTimer = state->sceneTimer;
        g_CameraViewMode = state->cameraViewMode;
        g_RacePhase = state->racePhase;
        g_RaceCueFlags = state->raceCueFlags;
        g_RivalCueFlags = state->rivalCueFlags;
        g_RivalCueCooldown0 = state->rivalCueCooldown[0];
        g_RivalCueCooldown1 = state->rivalCueCooldown[1];
        g_RivalCueCooldown2 = state->rivalCueCooldown[2];
        g_RivalCueCooldown3 = state->rivalCueCooldown[3];
        break;
    case RACE_BOOTSTRAP_POST_SCENERY:
        g_PauseDebounce = state->pauseDebounce;
        g_RaceFadeTimer = state->raceFadeTimer;
        break;
    case RACE_BOOTSTRAP_CONTROLS:
        g_RivalCueEnabled = state->rivalCueEnabled;
        g_PlayerAutoSteer = state->playerAutoSteer;
        g_RaceCueDelay = state->raceCueDelay;
        if (retireCameraActive != 0)
            *retireCameraActive = state->retireCameraActive;
        break;
    case RACE_BOOTSTRAP_ACTIVATE:
        g_FrameSyncThreshold = state->frameSyncThreshold;
        break;
    }
}
