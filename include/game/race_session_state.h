#ifndef GAME_RACE_SESSION_STATE_H
#define GAME_RACE_SESSION_STATE_H

#include "common.h"
#include "game/camera_types.h"

typedef struct RaceSessionState {
    s32 lapCount;
    s32 lapTimeMs;
    s32 lapTimeSaturated;
    s32 sectorEndDistance[3];
    s32 sectorIndex;
    s32 raceTimeRemaining;
    s32 raceTotalTime;
    s32 animTimer;
    s32 sceneTimer;
    CameraViewMode cameraViewMode;
    s32 racePhase;
    s32 raceCueFlags;
    s32 rivalCueFlags;
    s32 rivalCueCooldown[4];
    s32 pauseDebounce;
    s32 raceFadeTimer;
    s32 rivalCueEnabled;
    s32 playerAutoSteer;
    s32 raceCueDelay;
    s32 retireCameraActive;
    s32 frameSyncThreshold;
} RaceSessionState;

RaceSessionState RaceSessionStateDefaults(s32 courseIndex, s32 trackLength);
void RaceSessionStateReset(
    RaceSessionState *state, s32 courseIndex, s32 trackLength);

#endif
