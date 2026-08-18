#include "game/rival_update.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"

void RunRivalPlanningPasses(CarSimulation *simulation,
                            const RivalUpdatePolicy *policy) {
    s32 index;

    if (policy->useRubberBand) RankContenders();
    for (index = 0; index < simulation->carCount; index++) {
        if (RivalShouldUpdateTraffic(index, simulation->animationTimer, policy) &&
            simulation->cars[index].activeFlag != -1)
            UpdateCarTrafficAvoidance(&simulation->cars[index], index);
    }
    for (index = 0; index < simulation->carCount - 1; index++)
        CollideRivalCars(&simulation->cars[index], index);
    for (index = 0; index < simulation->carCount; index++) {
        UpdateCarAiTargetSpeed(&simulation->cars[index], index);
        ApplyCarRacingLineHint(&simulation->cars[index], index);
        ClampCarLateralOffset(&simulation->cars[index], index);
        SteerCarAlongRoute(&simulation->cars[index]);
    }
    if (policy->useRubberBand) {
        UpdateRivalRubberBand();
        for (index = (s16)g_ClosestRivalRank; index > 0; index--)
            SlowRivalAhead(g_RankedCars[index], index);
    }
}
