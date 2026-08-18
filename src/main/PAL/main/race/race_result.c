#include "game/race_result.h"

RaceResult RaceResultFromFinish(s32 finished, s32 racePosition) {
    if (!finished) return RACE_RESULT_IN_PROGRESS;
    return racePosition < 4 ? RACE_RESULT_WON : RACE_RESULT_LOST;
}
