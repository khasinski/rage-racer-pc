#include "game/angle.h"
#include "sin_table.inc"

s32 SinAngle(s32 angle) {
    const u32 a = (u32)angle & ANGLE_MASK;
    if (a <= ANGLE_QUARTER_TURN) return kSinQuarter[a];
    if (a <= ANGLE_HALF_TURN) return kSinQuarter[ANGLE_HALF_TURN - a];
    if (a <= ANGLE_THREE_QUARTER_TURN) return -kSinQuarter[a - ANGLE_HALF_TURN];
    return -kSinQuarter[ANGLE_FULL_TURN - a];
}
