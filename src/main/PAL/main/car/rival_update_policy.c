#include "game/rival_update.h"

s32 RivalShouldUpdateTraffic(s32 carIndex, s32 animationTimer,
                             const RivalUpdatePolicy *policy) {
    return !policy->timeSliceDistantCars || carIndex < 4 ||
           (carIndex & 1) == (animationTimer & 1);
}
