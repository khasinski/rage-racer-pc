#ifndef GAME_REPLAY_H
#define GAME_REPLAY_H

#include "common.h"

enum {
    REPLAY_RIVAL_COUNT = 11,
    REPLAY_SUBFRAMES_PER_SAMPLE = 2,
    GRAND_PRIX_REPLAY_SAMPLE_COUNT = 0x2EE,
    TIME_ATTACK_REPLAY_SAMPLE_COUNT = 0x505,
    GRAND_PRIX_REPLAY_SUBFRAME_COUNT =
        GRAND_PRIX_REPLAY_SAMPLE_COUNT * REPLAY_SUBFRAMES_PER_SAMPLE,
    TIME_ATTACK_REPLAY_SUBFRAME_COUNT =
        TIME_ATTACK_REPLAY_SAMPLE_COUNT * REPLAY_SUBFRAMES_PER_SAMPLE,
};

typedef struct ReplayCarFrame {
    s32 x;
    s32 y;
    s32 z;
    s32 modelY;
    s32 bodyPitch;
    s32 bodyYaw;
    s32 bodyRoll;
    s32 wheelRotation;
    s32 trackPointIndex;
    s16 steeringAngle;
    s16 modelIndex;
    s16 activeFlag;
    s16 aiEnabled;
    s16 brakeInput;
} ReplayCarFrame;

typedef struct ReplayGrandPrixFrame {
    ReplayCarFrame player;
    ReplayCarFrame rivals[REPLAY_RIVAL_COUNT];
    s32 tiltCounter;
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
    s16 brakeInput;
} ReplayTimeAttackFrame;

#endif
