#ifndef GAME_LAP_TRACKER_H
#define GAME_LAP_TRACKER_H

#include "common.h"

typedef struct LapTrackerInput {
    s32 currentLap;
    s32 targetLapCount;
    s32 totalProgress;
    s32 trackLength;
} LapTrackerInput;

typedef struct LapTrackerDecision {
    s32 crossedLine;
    s32 nextLap;
    s32 finished;
} LapTrackerDecision;

LapTrackerDecision LapTrackerEvaluate(const LapTrackerInput *input);

#endif
