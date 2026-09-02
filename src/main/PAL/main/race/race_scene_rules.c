#include "game/race_scene_internal.h"
#include "game/state.h"

enum {
    STANDARD_RACE_LAPS = 3,
    LONG_COURSE_INDEX = 3,
    LONG_RACE_LAPS = 6,
    CAMERA_MAPPING_INDEX = 6,
    NEGCON_MAPPING_OFFSET = 8,
};

s32 RaceLapCount(s32 courseIndex) {
    return courseIndex == LONG_COURSE_INDEX ? LONG_RACE_LAPS
                                            : STANDARD_RACE_LAPS;
}

void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]) {
    sectorEnds[0] = trackLength / 3;
    sectorEnds[1] = sectorEnds[0] * 2;
    sectorEnds[2] = trackLength;
}

u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]) {
    s32 mappingOffset =
        padType == PAD_TYPE_NEGCON ? NEGCON_MAPPING_OFFSET : 0;

    return buttonMapping[CAMERA_MAPPING_INDEX + mappingOffset];
}

s32 CanPauseRace(s16 phase) { return phase == 1 || phase == 2; }

s32 CanToggleRaceCamera(s16 phase) { return phase == 2 || phase == 3; }

s32 LastRacePauseOption(s16 grandPrixMode) {
    return grandPrixMode != 0 ? 1 : 2;
}

RacePauseAction DecideRacePauseAction(s16 phase, s16 grandPrixMode,
                                      s16 cursor) {
    if (cursor == LastRacePauseOption(grandPrixMode)) {
        if (grandPrixMode == 0 || phase < 2) {
            return RACE_PAUSE_QUIT;
        }
        return RACE_PAUSE_RETIRE;
    }
    if (cursor == 1 && grandPrixMode == 0) {
        return RACE_PAUSE_RESTART;
    }
    return RACE_PAUSE_RESUME;
}
