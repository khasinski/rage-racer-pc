#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"

/*
 * Samples a point two segments ahead (or behind in the reverse series), moves
 * it sideways onto the rival's racing line, then turns the car towards it.
 */
void SteerCarAlongRoute(GameCarRuntime *car) {
    const GameTrackPoint *point;
    s32 raceSeries = g_RaceSeries != 0;
    s32 index;
    s32 lateral = car->aiLateralOffset;
    s32 targetAngle;
    s32 trackFacing;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    index = car->trackPointIndex + (raceSeries ? 2 : -2);
    index = WrapTrackPointIndex(index);

    point = TrackPoint(index);
    if (lateral > point->rightHalfWidth) {
        lateral = point->rightHalfWidth * car->normalizedLateralOffset / 2048;
    } else if (lateral < -point->leftHalfWidth) {
        lateral = -point->leftHalfWidth * car->normalizedLateralOffset / 2048;
    }

    targetAngle = CalculateTrackOffsetHeading(
        index, car->segmentFraction, car->x, car->z, lateral);
    trackFacing = raceSeries * ANGLE_HALF_TURN +
                  ANGLE_THREE_QUARTER_TURN - car->trackHeading.value;
    car->steeringAngle = -GetAngleDelta(trackFacing, targetAngle) * 3;

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        car->headingAngle += GetAngleDelta(car->headingAngle, targetAngle);
        car->targetYaw = car->headingAngle;
        car->bodyYaw = car->headingAngle;
    }
}
