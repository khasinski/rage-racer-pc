#include "game/car_track_internal.h"

s32 CalculateRouteOffsetHeading(const TrackRoute *route, s32 pointIndex,
                                 s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset) {
    LVec target;
    s32 trackAngle;
    s32 lateralStep;
    s32 dx;
    s32 dz;

    InterpolateRoutePoint(route, pointIndex, &target, segmentFraction);
    trackAngle = WrapSigned32(
        (int64_t)ANGLE_FULL_TURN -
        SmoothRouteAngle(route, pointIndex, segmentFraction));
    lateralStep = WrapSigned32((int64_t)SinAngle(trackAngle) * lateralOffset) /
                  ANGLE_FULL_TURN;
    target.x = WrapSigned32((int64_t)target.x + lateralStep);
    lateralStep = WrapSigned32((int64_t)CosAngle(trackAngle) * lateralOffset) /
                  ANGLE_FULL_TURN;
    target.z = WrapSigned32((int64_t)target.z + lateralStep);
    dx = WrapSigned32((int64_t)target.x - carX);
    dz = WrapSigned32((int64_t)target.z - carZ);

    return WrapSigned32((int64_t)ANGLE_QUARTER_TURN - Atan2(dx, dz));
}

/* Aim at an offset point on the centre-line and turn towards it. */
void SteerCarOnRoute(PlayerCarRuntime *car, const GameCarSpec *spec,
                      const TrackRoute *route) {
    s32 lateral;
    s32 aheadIndex;
    s32 wantedHeading;

    if (car == NULL || spec == NULL || route == NULL ||
        route->count <= 0 || route->points == NULL) {
        return;
    }

    lateral = car->trackLateralOffset;

    /* A backwards-launched car follows the centre-line in reverse. */
    aheadIndex = RouteIndex(route, WrapSigned32(
        (int64_t)car->trackPointIndex +
        (car->drive.launchDirection != 0 ? 2 : -2)));

    wantedHeading = CalculateRouteOffsetHeading(route, 
        aheadIndex, car->segmentFraction, car->x, car->z, lateral);

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        /* Preserve the recovered signed 16-bit view of the response. */
        s32 response = WrapSigned16(spec->steerResponse);
        s32 towards;

        if (response <= 0) {
            response = 1;
        }
        towards = GetAngleDelta(car->headingAngle, wantedHeading);
        car->headingAngle = WrapSigned32(
            (int64_t)car->headingAngle +
            WrapSigned32((int64_t)towards * 20) / response);
    }
}
