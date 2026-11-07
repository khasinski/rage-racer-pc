#include "game/rival.h"
#include "game/car_motion_internal.h"
#include "game/car_track_internal.h"

void PlaceRival(GameCarRuntime *car, const TrackRoute *route, int reverse) {
    if (car == NULL || car->activeFlag == -1 || route == NULL ||
        route->points == NULL || route->count <= 0 || route->length <= 0) return;
    const CarTrackLimits limits = {.rightInset = 60, .leftInset = -60};
    if (car->motionActive) ApplyCarKnockback(car);
    StepCarTrackState(car, route, car->trackPointIndex, &limits, reverse, 0);
}
