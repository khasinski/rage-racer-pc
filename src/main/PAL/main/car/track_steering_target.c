#include "game/angle.h"
#include "game/car_internal.h"
#include "game/render.h"
#include "game/track.h"

s32 CalculateTrackOffsetHeading(s32 pointIndex, s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset) {
    s32 target[3];
    s32 trackAngle;

    InterpolateTrackPoint(pointIndex, target, segmentFraction);
    trackAngle = ANGLE_FULL_TURN -
                 SmoothTrackAngle(pointIndex, segmentFraction);
    target[0] += rsin(trackAngle) * lateralOffset / 4096;
    target[2] += rcos(trackAngle) * lateralOffset / 4096;

    return ANGLE_QUARTER_TURN - Atan2(target[0] - carX, target[2] - carZ);
}
