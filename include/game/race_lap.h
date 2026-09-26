#ifndef GAME_RACE_LAP_H
#define GAME_RACE_LAP_H

#include "game/car.h"

typedef enum LapEvent {
    LAP_NONE,
    LAP_STARTED,
    LAP_COMPLETED,
    LAP_FINISHED,
} LapEvent;

/* Lap zero is the grid. Each call advances at most one lap, preserving the
 * retail crossing rule. Distance is accumulated progress, not wrapped position. */
LapEvent AdvanceCarLap(PlayerCarRuntime *car, s32 trackLength, s32 lapCount);
int IsCarLapBehind(const PlayerCarRuntime *car, s32 trackLength);

#endif
