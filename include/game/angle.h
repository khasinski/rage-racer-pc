#ifndef GAME_ANGLE_H
#define GAME_ANGLE_H

#include "common.h"

enum Angle {
    ANGLE_MASK = 0xFFF,
    ANGLE_QUARTER_TURN = 0x400,
    ANGLE_HALF_TURN = 0x800,
    ANGLE_THREE_QUARTER_TURN = 0xC00,
    ANGLE_FULL_TURN = 0x1000
};

s32 GetAngleDistance(s32 from, s32 to);
s32 GetAngleDelta(s32 from, s32 to);

#endif
