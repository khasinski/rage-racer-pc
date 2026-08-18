#ifndef GAME_RACE_RESULT_H
#define GAME_RACE_RESULT_H

#include "common.h"

typedef enum RaceResult {
    RACE_RESULT_IN_PROGRESS,
    RACE_RESULT_WON,
    RACE_RESULT_LOST
} RaceResult;

RaceResult RaceResultFromFinish(s32 finished, s32 racePosition);

#endif
