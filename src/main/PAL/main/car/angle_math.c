#include "game/angle.h"

static s32 NormalizeAngleDelta(s32 from, s32 to) {
    s32 delta = (to & ANGLE_MASK) - (from & ANGLE_MASK);

    if (delta > ANGLE_HALF_TURN) {
        delta -= ANGLE_FULL_TURN;
    } else if (delta < -ANGLE_HALF_TURN) {
        delta += ANGLE_FULL_TURN;
    }
    return delta;
}

/* Shortest unsigned distance between two 12-bit angles, in [0, 0x800]. */
s32 GetAngleDistance(s32 from, s32 to) {
    s32 delta = NormalizeAngleDelta(from, to);

    return delta < 0 ? -delta : delta;
}

/*
 * Signed shortest angular delta between two 12-bit angles (from -> to),
 * in [-0x800, 0x800]. An exact half-turn retains the sign of `to - from` after
 * masking. GetAngleDistance returns its unsigned magnitude.
 */
s32 GetAngleDelta(s32 from, s32 to) {
    return NormalizeAngleDelta(from, to);
}
