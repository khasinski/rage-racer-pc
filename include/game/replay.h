#ifndef GAME_REPLAY_H
#define GAME_REPLAY_H

#include "common.h"

enum {
    REPLAY_SUBFRAMES_PER_SAMPLE = 2,
    GRAND_PRIX_REPLAY_SAMPLE_COUNT = 0x2EE,
    TIME_ATTACK_REPLAY_SAMPLE_COUNT = 0x505,
    GRAND_PRIX_REPLAY_SUBFRAME_COUNT =
        GRAND_PRIX_REPLAY_SAMPLE_COUNT * REPLAY_SUBFRAMES_PER_SAMPLE,
    TIME_ATTACK_REPLAY_SUBFRAME_COUNT =
        TIME_ATTACK_REPLAY_SAMPLE_COUNT * REPLAY_SUBFRAMES_PER_SAMPLE,
};

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
