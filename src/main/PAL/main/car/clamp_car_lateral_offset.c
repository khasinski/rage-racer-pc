#include "game/car.h"
#include "game/rival.h"
#include "game/car_track_internal.h"
#include "game/track.h"

enum {
    FRONT_LINE_NUMERATOR = 5,
    FRONT_LINE_DENOMINATOR = 8,
    REAR_LINE_NUMERATOR = 4,
    REAR_LINE_DENOMINATOR = 7,
};

/*
 * Clamps the car's lateral offset (aiLateralOffset) to a fraction of the track
 * half-width at its current point: `leftHalfWidth` when offset is negative,
 * `rightHalfWidth` otherwise. The front four rival slots use 5/8 of the
 * half-width; the remaining slots use 4/7. The value is written back only
 * when it exceeds that limit.
 */
void ClampRivalLine(GameCarRuntime *car, s32 rivalSlot, const TrackRoute *route) {
    s32 current;
    s32 magnitude;
    s32 halfWidth;
    s32 limit;
    const GameTrackPoint *point;

    if (car == NULL || route == NULL || route->count <= 0 || route->points == NULL || rivalSlot < 0 ||
        rivalSlot >= RACE_CAR_SLOT_COUNT) {
        return;
    }

    current = car->aiLateralOffset;
    magnitude = current < 0 ? -current : current;
    point = RoutePoint(route, car->trackPointIndex);
    halfWidth = current < 0 ? point->leftHalfWidth : point->rightHalfWidth;
    if (halfWidth < 0) {
        halfWidth = 0;
    }
    limit = rivalSlot < RIVAL_CONTENDER_COUNT
        ? (halfWidth * FRONT_LINE_NUMERATOR) / FRONT_LINE_DENOMINATOR
        : (halfWidth * REAR_LINE_NUMERATOR) / REAR_LINE_DENOMINATOR;

    if (limit < magnitude) {
        car->aiLateralOffset = (s16)(current > 0 ? limit : -limit);
    }
}
