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
/* Fixed-point angle in 1/4096 turns. Argument order is (x, y), unlike C atan2. */
s32 Atan2(s32 x, s32 y);
/* Pure canonical fixed-point trig, usable without PsyZ. */
s32 SinAngle(s32 angle);
s32 CosAngle(s32 angle);
s32 rsin(s32 angle);
s32 rcos(s32 angle);

#endif
