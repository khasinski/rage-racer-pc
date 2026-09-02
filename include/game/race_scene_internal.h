#ifndef GAME_RACE_SCENE_INTERNAL_H
#define GAME_RACE_SCENE_INTERNAL_H

#include "common.h"

s32 RaceLapCount(s32 courseIndex);
void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]);
u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]);
s32 CanPauseRace(s16 phase);
s32 CanToggleRaceCamera(s16 phase);
s32 LastRacePauseOption(s16 grandPrixMode);

#endif
