#ifndef GAME_RIVAL_UPDATE_H
#define GAME_RIVAL_UPDATE_H

#include "common.h"
#include "game/car_types.h"

typedef struct CarSimulation {
    GameCarRuntime *cars;
    s32 carCount;
    s32 animationTimer;
} CarSimulation;

typedef struct RivalUpdatePolicy {
    s32 useRubberBand;
    s32 timeSliceDistantCars;
    s32 wrapTrackProgress;
    s32 enableRaceBoost;
} RivalUpdatePolicy;

s32 RivalShouldUpdateTraffic(s32 carIndex, s32 animationTimer,
                             const RivalUpdatePolicy *policy);
void RunRivalPlanningPasses(CarSimulation *simulation,
                            const RivalUpdatePolicy *policy);
void IntegrateRivalSpeeds(CarSimulation *simulation,
                          const RivalUpdatePolicy *policy);
void IntegrateRivalSpeed(GameCarRuntime *car,
                         const RivalUpdatePolicy *policy);

#endif
