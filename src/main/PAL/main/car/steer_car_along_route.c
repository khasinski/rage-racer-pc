#include "game/angle.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

static s32 Fixed12ToInteger(s32 value) {
    if (value < 0) {
        value += (1 << 12) - 1;
    }
    return value >> 12;
}

/*
 * Samples a point two segments ahead (or behind in the reverse series), moves
 * it sideways onto the rival's racing line, then turns the car towards it.
 */
void SteerCarAlongRoute(GameCarRuntime *car) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    GameTrackPoint *point;
    s32 raceSeries = ReadStableRaceSeries() != 0;
    s32 index;
    s32 lateral = car->aiLateralOffset;
    s32 coords[3];
    s32 targetAngle;
    s32 trackFacing;

    car->reservedDC = 0;
    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    index = car->trackPointIndex + (raceSeries ? 2 : -2);
    index %= g_TrackPointCount;
    if (index < 0) {
        index += g_TrackPointCount;
    }

    point = TrackPoint(index);
    if (lateral > point->rightHalfWidth) {
        lateral = point->rightHalfWidth * car->normalizedLateralOffset / 2048;
    } else if (lateral < -point->leftHalfWidth) {
        lateral = -point->leftHalfWidth * car->normalizedLateralOffset / 2048;
    }

    InterpolateTrackPoint(index, coords, car->segmentFraction);
    trackFacing = ANGLE_FULL_TURN -
                  SmoothTrackAngle(index, car->segmentFraction);
    coords[0] += Fixed12ToInteger(rsin(trackFacing) * lateral);
    coords[2] += Fixed12ToInteger(rcos(trackFacing) * lateral);

    targetAngle = ANGLE_QUARTER_TURN -
                  Atan2(coords[0] - car->x, coords[2] - car->z);
    trackFacing = raceSeries * ANGLE_HALF_TURN +
                  ANGLE_THREE_QUARTER_TURN - car->trackHeading.value;
    car->steeringAngle = -GetAngleDelta(trackFacing, targetAngle) * 3;

    if (car->verticalMotionState == 0) {
        car->headingAngle += GetAngleDelta(car->headingAngle, targetAngle);
        ai->targetYaw = car->headingAngle;
        car->bodyYaw = car->headingAngle;
    }
}
