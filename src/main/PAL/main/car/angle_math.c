#include "common.h"
#include "game/angle.h"

/*
 * Unsigned angular difference between two 12-bit angles, wrapped to [0, 0x800]
 * (takes the shorter way around the circle). Signed companion is
 * GetAngleDelta.
 */
s32 GetAngleDistance(s32 from, s32 to) {
    s32 lhs = from & ANGLE_MASK;
    s32 rhs = to & ANGLE_MASK;
    s32 diff;

    if (lhs < rhs) {
        diff = rhs - lhs;
    } else {
        diff = lhs - rhs;
    }

    if (diff > ANGLE_HALF_TURN) {
        diff = ANGLE_FULL_TURN - diff;
    }

    return diff;
}

/*
 * Signed shortest angular delta between two 12-bit angles (from -> to),
 * result in (-0x800, 0x800]. Signed companion of GetAngleDistance (which returns
 * the unsigned magnitude).
 */
s32 GetAngleDelta(s32 from, s32 to) {
    s32 lhs = from & ANGLE_MASK;
    s32 rhs = to & ANGLE_MASK;
    s32 sign = lhs < rhs;
    s32 diff;

    if (sign) {
        diff = rhs - lhs;
    } else {
        diff = lhs - rhs;
    }

    if (diff > ANGLE_HALF_TURN) {
        diff = ANGLE_FULL_TURN - diff;
        sign = !sign;
    }

    {
        s32 ret = diff;

        if (!sign) {
            ret = -ret;
        }

        return ret;
    }
}
