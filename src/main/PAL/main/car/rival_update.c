#include "game/rival_update.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"

#define RIVAL_CAR_COUNT 11

void RunRivalPlanningPasses(const RivalUpdatePolicy *policy) {
    s32 index;

    if (policy->useRubberBand) RankContenders();
    for (index = 0; index < RIVAL_CAR_COUNT; index++) {
        if (RivalShouldUpdateTraffic(index, g_AnimTimer, policy) &&
            g_Cars[index].activeFlag != -1)
            UpdateCarTrafficAvoidance(&g_Cars[index], index);
    }
    for (index = 0; index < RIVAL_CAR_COUNT - 1; index++)
        CollideRivalCars(&g_Cars[index], index);
    for (index = 0; index < RIVAL_CAR_COUNT; index++) {
        UpdateCarAiTargetSpeed(&g_Cars[index], index);
        ApplyCarRacingLineHint(&g_Cars[index], index);
        ClampCarLateralOffset(&g_Cars[index], index);
        SteerCarAlongRoute(&g_Cars[index]);
    }
    if (policy->useRubberBand) {
        UpdateRivalRubberBand();
        for (index = (s16)g_ClosestRivalRank; index > 0; index--)
            SlowRivalAhead(g_RankedCars[index], index);
    }
}

void IntegrateRivalSpeeds(const RivalUpdatePolicy *policy) {
    s32 index;
    for (index = 0; index < RIVAL_CAR_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);
        if (car->activeFlag == -1) continue;

        if (policy->enableRaceBoost && car->boostTimer > 0) {
            if (car->boostAccelerationThreshold < car->boostTimer &&
                car->speed >= 0x321) {
                car->acceleration = 0;
            } else if (ai->accelerationLimit >= car->acceleration) {
                car->acceleration += ai->boostAcceleration;
            } else {
                car->acceleration = ai->accelerationLimit;
            }
            ai->boostTimer--;
        } else if ((policy->enableRaceBoost &&
                    car->acceleration <= car->accelerationLimit) ||
                   (!policy->enableRaceBoost &&
                    car->acceleration < car->accelerationLimit)) {
            car->acceleration += car->accelerationStep;
        } else {
            car->acceleration = car->accelerationLimit;
        }
        car->speed = car->speed * 94 / 100 + car->acceleration;
        car->bodyYaw += GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5;
    }
}
