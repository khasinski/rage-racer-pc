#ifndef GAME_RACE_SCENE_INTERNAL_H
#define GAME_RACE_SCENE_INTERNAL_H

#include "common.h"

typedef enum RacePauseAction {
    RACE_PAUSE_RESUME,
    RACE_PAUSE_QUIT,
    RACE_PAUSE_RESTART,
    RACE_PAUSE_RETIRE,
} RacePauseAction;

s32 RaceLapCount(s32 courseIndex);
void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]);
u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]);
s32 CanPauseRace(s16 phase);
s32 CanToggleRaceCamera(s16 phase);
s32 LastRacePauseOption(s16 grandPrixMode);
RacePauseAction DecideRacePauseAction(s16 phase, s16 grandPrixMode,
                                      s16 cursor);

#endif
