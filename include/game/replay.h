#ifndef GAME_REPLAY_H
#define GAME_REPLAY_H

#include "common.h"

typedef struct ReplayCarState {
    s32 x;
    s32 y;
    s32 z;
    u8 pad0C[0x14];
    s32 bodyPitch;
    s32 bodyYaw;
    s32 bodyRoll;
    u8 pad2C[4];
    s32 trackPointIndex;
    u8 pad34[0x10];
    s32 steeringAngle;
    s32 wheelRotation;
    u8 pad4C[0x14];
    s32 modelY;
    u8 pad64[0x28];
    s16 tiltCounter;
    u8 pad8E[0x20];
    s16 modelIndex;
} ReplayCarState;

typedef struct ReplayGrandPrixFrame {
    u16 x0;
    s16 y0;
    u16 z0;
    s16 modelY0;
    s16 bodyPitch0;
    s16 bodyYaw0;
    s16 bodyRoll0;
    s16 wheelRotation0;
    u16 x1;
    s16 y1;
    u16 z1;
    s16 modelY1;
    s16 bodyPitch1;
    s16 bodyYaw1;
    s16 bodyRoll1;
    s16 wheelRotation1;
    s32 tiltCounter;
    s32 trackPointIndex0;
    s32 trackPointIndex1;
    s16 steeringAngle0;
    s16 steeringAngle1;
} ReplayGrandPrixFrame;

typedef struct ReplayTimeAttackFrame {
    u16 x;
    s16 y;
    u16 z;
    s16 modelY;
    s16 bodyPitch;
    s16 bodyYaw;
    s16 bodyRoll;
    s16 wheelRotation;
    s32 tiltCounter;
    s32 trackPointIndex;
    s16 steeringAngle;
} ReplayTimeAttackFrame;

#endif
