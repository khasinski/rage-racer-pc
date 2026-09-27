#include "game/angle.h"
#include "game/car.h"
#include "game/rival.h"
#include "game/car_track_internal.h"
#include "game/integer.h"
#include "game/track.h"

enum {
    ROUTE_LOOKAHEAD_SEGMENTS = 2,
    NORMALIZED_LATERAL_DIVISOR = ANGLE_HALF_TURN,
    RIVAL_STEERING_DISPLAY_SCALE = 3,
};

/*
 * Samples a point two segments ahead (or behind in the reverse series), moves
 * it sideways onto the rival's racing line, then turns the car towards it.
 */
void SteerRival(GameCarRuntime *car, const TrackRoute *route, int reverse) {
    const GameTrackPoint *point;
    const s32 raceSeries = reverse != 0;
    s32 index;
    s32 lateral;
    s32 targetAngle;
    s32 trackFacing;

    if (car == NULL || route == NULL || route->count <= 0 || route->points == NULL) {
        return;
    }

    lateral = car->aiLateralOffset;
    index = WrapSigned32(
        (int64_t)car->trackPointIndex +
        (raceSeries ? ROUTE_LOOKAHEAD_SEGMENTS
                    : -ROUTE_LOOKAHEAD_SEGMENTS));
    index = RouteIndex(route, index);

    point = RoutePoint(route, index);
    if (lateral > point->rightHalfWidth) {
        lateral = WrapSigned32(
            (int64_t)point->rightHalfWidth *
            car->normalizedLateralOffset) / NORMALIZED_LATERAL_DIVISOR;
    } else if (lateral < -point->leftHalfWidth) {
        lateral = WrapSigned32(
            -(int64_t)point->leftHalfWidth *
            car->normalizedLateralOffset) / NORMALIZED_LATERAL_DIVISOR;
    }

    targetAngle = CalculateRouteOffsetHeading(
        route, index, car->segmentFraction, car->x, car->z, lateral);
    trackFacing = WrapSigned32(
        (int64_t)raceSeries * ANGLE_HALF_TURN +
        ANGLE_THREE_QUARTER_TURN - car->trackHeading);
    car->steeringAngle = WrapSigned32(
        (int64_t)WrapSigned32(
            -(int64_t)GetAngleDelta(trackFacing, targetAngle)) *
        RIVAL_STEERING_DISPLAY_SCALE);

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        car->headingAngle = WrapSigned32(
            (int64_t)car->headingAngle +
            GetAngleDelta(car->headingAngle, targetAngle));
        car->targetYaw = car->headingAngle;
        car->bodyYaw = car->headingAngle;
    }
}
