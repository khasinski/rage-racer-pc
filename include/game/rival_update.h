#ifndef GAME_RIVAL_UPDATE_H
#define GAME_RIVAL_UPDATE_H

#include "common.h"

typedef struct RivalUpdatePolicy {
    s32 useRubberBand;
    s32 timeSliceDistantCars;
    s32 wrapTrackProgress;
    s32 enableRaceBoost;
} RivalUpdatePolicy;

s32 RivalShouldUpdateTraffic(s32 carIndex, s32 animationTimer,
                             const RivalUpdatePolicy *policy);
void RunRivalPlanningPasses(const RivalUpdatePolicy *policy);
void IntegrateRivalSpeeds(const RivalUpdatePolicy *policy);

#endif
