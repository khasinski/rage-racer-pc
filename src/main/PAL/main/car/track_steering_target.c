#include "game/angle.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/render.h"
#include "game/track.h"
#include "game/track_internal.h"

s32 CalculateTrackOffsetHeading(s32 pointIndex, s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset) {
    LVec target;
    s32 trackAngle;
    s32 lateralStep;
    s32 dx;
    s32 dz;

    InterpolateTrackPoint(pointIndex, &target, segmentFraction);
    trackAngle = WrapSigned32(
        (int64_t)ANGLE_FULL_TURN -
        SmoothTrackAngle(pointIndex, segmentFraction));
    lateralStep = WrapSigned32((int64_t)rsin(trackAngle) * lateralOffset) /
                  ANGLE_FULL_TURN;
    target.x = WrapSigned32((int64_t)target.x + lateralStep);
    lateralStep = WrapSigned32((int64_t)rcos(trackAngle) * lateralOffset) /
                  ANGLE_FULL_TURN;
    target.z = WrapSigned32((int64_t)target.z + lateralStep);
    dx = WrapSigned32((int64_t)target.x - carX);
    dz = WrapSigned32((int64_t)target.z - carZ);

    return WrapSigned32((int64_t)ANGLE_QUARTER_TURN - Atan2(dx, dz));
}
