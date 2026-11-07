#include "game/angle.h"

s32 CosAngle(s32 angle) {
    return SinAngle(((u32)angle + ANGLE_QUARTER_TURN) & ANGLE_MASK);
}
