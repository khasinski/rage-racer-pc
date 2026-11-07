#include "game/car_track_internal.h"
#include "game/geometry.h"

static int TrackSegmentContainsCar(const GameCarRuntime *car,
                                   const TrackRoute *route, s32 index) {
    DVecValue carPosition;
    DVecValue nearLeftCorner;
    DVecValue nearRightCorner;
    DVecValue farLeftCorner;
    DVecValue farRightCorner;
    const GameTrackPoint *near = RoutePoint(route, index);
    const GameTrackPoint *far = RoutePoint(route, WrapSigned32((int64_t)index + 1));
    s32 segmentX = WrapSigned32((int64_t)far->x - near->x);
    s32 segmentZ = WrapSigned32((int64_t)far->z - near->z);
    s32 nearCos = CosAngle(ANGLE_THREE_QUARTER_TURN - near->angle);
    s32 nearSin = SinAngle(ANGLE_THREE_QUARTER_TURN - near->angle);
    s32 farCos = CosAngle(ANGLE_THREE_QUARTER_TURN - far->angle);
    s32 farSin = SinAngle(ANGLE_THREE_QUARTER_TURN - far->angle);
    s32 nearLeft = WrapSigned16(near->leftHalfWidth * 2);
    s32 nearRight = WrapSigned16(near->rightHalfWidth * 2);
    s32 farLeft = WrapSigned16(far->leftHalfWidth * 2);
    s32 farRight = WrapSigned16(far->rightHalfWidth * 2);

    /* All corners use the near centre-line point as their origin. */
    carPosition.components.vx = WrapSigned16((int64_t)car->x - near->x);
    carPosition.components.vy = WrapSigned16((int64_t)car->z - near->z);
    nearLeftCorner.components.vx = WrapSigned16(
        nearLeft * WrapSigned16(nearCos) / ANGLE_FULL_TURN);
    nearLeftCorner.components.vy = WrapSigned16(
        -nearLeft * WrapSigned16(nearSin) / ANGLE_FULL_TURN);
    nearRightCorner.components.vx = WrapSigned16(
        -nearRight * WrapSigned16(nearCos) / ANGLE_FULL_TURN);
    nearRightCorner.components.vy = WrapSigned16(
        nearRight * WrapSigned16(nearSin) / ANGLE_FULL_TURN);
    farLeftCorner.components.vx = WrapSigned16(
        (int64_t)segmentX +
        farLeft * WrapSigned16(farCos) / ANGLE_FULL_TURN);
    farLeftCorner.components.vy = WrapSigned16(
        (int64_t)segmentZ -
        farLeft * WrapSigned16(farSin) / ANGLE_FULL_TURN);
    farRightCorner.components.vx = WrapSigned16(
        (int64_t)segmentX -
        farRight * WrapSigned16(farCos) / ANGLE_FULL_TURN);
    farRightCorner.components.vy = WrapSigned16(
        (int64_t)segmentZ +
        farRight * WrapSigned16(farSin) / ANGLE_FULL_TURN);

    return TriangleArea(nearLeftCorner.packed, nearRightCorner.packed,
                      carPosition.packed) >= 0 &&
           TriangleArea(nearRightCorner.packed, farRightCorner.packed,
                      carPosition.packed) >= 0 &&
           TriangleArea(farRightCorner.packed, farLeftCorner.packed,
                      carPosition.packed) > 0 &&
           TriangleArea(farLeftCorner.packed, nearLeftCorner.packed,
                      carPosition.packed) >= 0;
}

/*
 * Finds the track segment whose rotated, half-width quad contains the car.
 * The search starts at the caller's best guess and alternates forward and
 * backward over neighbouring segments.
 */
s32 FindCarTrackSegment(const GameCarRuntime *car, const TrackRoute *route,
                         s32 startIndex) {
    s32 attempts;
    s32 index;
    s32 stride = 0;

    if (car == NULL || route == NULL || route->count <= 0 ||
        route->points == NULL) {
        return -1;
    }

    startIndex = RouteIndex(route, startIndex);
    index = startIndex;

    for (attempts = 0; attempts < route->count; attempts++) {
        if (TrackSegmentContainsCar(car, route, index)) {
            return index;
        }

        /* Preserve the recovered alternating order: start, +1, -1, +2, -2,
         * ... . Its first point-count entries visit every segment once. */
        stride++;
        index = WrapSigned32(
            (int64_t)index + ((stride % 2) != 0 ? stride : -stride));
        index = RouteIndex(route, index);
    }

    return -1;
}
