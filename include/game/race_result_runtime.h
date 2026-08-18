#ifndef GAME_RACE_RESULT_RUNTIME_H
#define GAME_RACE_RESULT_RUNTIME_H

#include "game/car_types.h"
#include "game/race_result.h"

void ApplyRaceFinishResult(PlayerCarRaceState *raceState,
                           s32 lapCount,
                           s32 grandPrixMode,
                           RaceResult result);

#endif
