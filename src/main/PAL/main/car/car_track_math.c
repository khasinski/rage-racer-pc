#include "game/angle.h"
#include "game/car_internal.h"
#include "game/integer.h"

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength) {
    s32 endContribution;
    s32 remaining;
    s32 startContribution;

    if (segmentLength <= 0) {
        return start;
    }
    endContribution = WrapSigned32((int64_t)end * alongSegment);
    remaining = WrapSigned32((int64_t)segmentLength - alongSegment);
    startContribution = WrapSigned32((int64_t)start * remaining);

    return WrapSigned32(
        (int64_t)endContribution + startContribution) / segmentLength;
}

s32 CarTrackFixed12ToInteger(s32 value) {
    if (value < 0) {
        value += ANGLE_MASK;
    }
    return value >> 12;
}

s32 ProjectCarTrackAxis(s32 value) {
    /* Retail first truncated the 12-bit fixed-point product, then divided the
     * scaled edge offset by four. Keep that two-stage negative rounding. */
    if (value < 0) {
        value += ANGLE_MASK;
    }
    return value >> 14;
}

s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan) {
    s32 start = pointHeading;
    s32 end = nextHeading;
    s32 endContribution;
    s32 remaining;
    s32 startContribution;

    if (arcSpan <= 0) {
        return pointHeading;
    }
    if (end - start > ANGLE_HALF_TURN) {
        end -= ANGLE_FULL_TURN;
    } else if (start - end > ANGLE_HALF_TURN) {
        start -= ANGLE_FULL_TURN;
    }
    endContribution = WrapSigned32((int64_t)end * swept);
    remaining = WrapSigned32((int64_t)arcSpan - swept);
    startContribution = WrapSigned32((int64_t)start * remaining);
    return WrapSigned16(
        WrapSigned32((int64_t)endContribution + startContribution) /
        arcSpan);
}
