#include "game/race_session_state.h"
#include "game/camera_types.h"

enum {
    RACE_LONG_COURSE_INDEX = 3,
    RACE_DEFAULT_LAPS = 3,
    RACE_LONG_COURSE_LAPS = 6,
    RACE_DEFAULT_TIME_REMAINING = 0x3A98,
    RACE_DEFAULT_RIVAL_CUES = 0x1FE,
    RACE_FRAME_SYNC_THRESHOLD = 0x180
};

RaceSessionState RaceSessionStateDefaults(s32 courseIndex, s32 trackLength) {
    RaceSessionState state = {0};

    state.lapCount = courseIndex == RACE_LONG_COURSE_INDEX
        ? RACE_LONG_COURSE_LAPS : RACE_DEFAULT_LAPS;
    state.sectorEndDistance[0] = trackLength / 3;
    state.sectorEndDistance[1] = state.sectorEndDistance[0] * 2;
    state.sectorEndDistance[2] = trackLength;
    state.sectorIndex = -2;
    state.raceTimeRemaining = RACE_DEFAULT_TIME_REMAINING;
    state.cameraViewMode = CAMERA_VIEW_CAR;
    state.rivalCueFlags = RACE_DEFAULT_RIVAL_CUES;
    state.rivalCueEnabled = 1;
    state.frameSyncThreshold = RACE_FRAME_SYNC_THRESHOLD;
    return state;
}

void RaceSessionStateReset(
    RaceSessionState *state, s32 courseIndex, s32 trackLength) {
    *state = RaceSessionStateDefaults(courseIndex, trackLength);
}
