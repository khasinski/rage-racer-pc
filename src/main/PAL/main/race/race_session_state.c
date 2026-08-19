#include "game/race_session_state.h"
#include "game/camera_types.h"

RaceSessionState RaceSessionStateDefaults(s32 courseIndex, s32 trackLength) {
    RaceSessionState state = {0};

    state.lapCount = courseIndex == 3 ? 6 : 3;
    state.sectorEndDistance[0] = trackLength / 3;
    state.sectorEndDistance[1] = state.sectorEndDistance[0] * 2;
    state.sectorEndDistance[2] = trackLength;
    state.sectorIndex = -2;
    state.raceTimeRemaining = 0x3A98;
    state.cameraViewMode = CAMERA_VIEW_CAR;
    state.rivalCueFlags = 0x1FE;
    state.rivalCueEnabled = 1;
    state.frameSyncThreshold = 0x180;
    return state;
}

void RaceSessionStateReset(
    RaceSessionState *state, s32 courseIndex, s32 trackLength) {
    *state = RaceSessionStateDefaults(courseIndex, trackLength);
}
