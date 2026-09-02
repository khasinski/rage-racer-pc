#include "game/angle.h"
#include "game/car_internal.h"

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength) {
    return (end * alongSegment + start * (segmentLength - alongSegment)) /
           segmentLength;
}

s32 CarTrackFixed12ToInteger(s32 value) {
    if (value < 0) {
        value += ANGLE_MASK;
    }
    return value >> 12;
}

s32 ProjectCarTrackAxis(s32 value) {
    if (value < 0) {
        value += ANGLE_MASK;
    }
    return value >> 14;
}

s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan) {
    if (nextHeading - pointHeading > ANGLE_HALF_TURN) {
        nextHeading -= ANGLE_FULL_TURN;
    } else if (pointHeading - nextHeading > ANGLE_HALF_TURN) {
        pointHeading -= ANGLE_FULL_TURN;
    }
    return (s16)((nextHeading * swept +
                  pointHeading * (arcSpan - swept)) / arcSpan);
}
