#include "game/angle.h"
#include "game/car.h"
#include "game/rival.h"
#include "game/car_track_internal.h"
#include "game/track.h"

#include <string.h>

int InitRival(GameCarRuntime *car, const TrackRoute *route,
                 const TrackRivalStart *start, s32 walkStart, int reverse, u16 model) {
    if (car == NULL || route == NULL || route->points == NULL || route->count <= 0 ||
        route->length <= 0 || start == NULL || (reverse != 0 && reverse != 1)) return 0;
    const s32 series = reverse;
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;
    s32 startPointIndex;

    memset(car, 0, sizeof(*car));
    car->initializedFlag = 1;
    car->aiEnabled = 1;
    car->facingBackwards = (s16)series;
    car->modelIndex = model;
    car->rivalModelId = model;
    startPointIndex = RouteIndex(route, start->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = start->x;
    car->z = start->z;

    trackPointIndex = FindCarTrackSegment(car, route, car->trackPointIndex);
    if (trackPointIndex < 0) {
        trackPointIndex = startPointIndex;
        car->x = RoutePoint(route, startPointIndex)->x;
        car->z = RoutePoint(route, startPointIndex)->z;
    }
    car->trackPointIndex = trackPointIndex;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    series * ANGLE_HALF_TURN -
                    RoutePoint(route, trackPointIndex)->angle) & ANGLE_MASK;
    car->baseBodyYaw = car->bodyYaw;
    car->targetYaw = car->bodyYaw;
    car->headingAngle = car->bodyYaw;
    SeedCarTrackProgress(car, route, walkStart, start->activeFlag, reverse);

    car->activeFlag = start->activeFlag;
    if (start->activeFlag != -1) {
        StepCarTrackState(car, route, car->trackPointIndex, &trackLimits, reverse, 0);
        car->modelY = car->y;
        car->previousTrackProgress = car->trackProgress;
    }

    car->initialLateralOffset = car->trackLateralOffset;
    car->avoidanceTargetOffset = car->trackLateralOffset;
    car->aiLateralOffset = car->trackLateralOffset;
    CopyCarBodyRotationToModel(car);
    car->modelY = car->y;
    return 1;
}
