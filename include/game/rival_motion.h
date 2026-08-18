#ifndef GAME_RIVAL_MOTION_H
#define GAME_RIVAL_MOTION_H

#include "common.h"

typedef struct RivalMotionState {
    s32 speed;
    s32 acceleration;
    s32 accelerationStep;
    s32 accelerationLimit;
    s32 boostTimer;
    s32 boostAccelerationThreshold;
    s32 boostAcceleration;
    s32 boostAccelerationLimit;
    s32 bodyYaw;
    s32 targetYaw;
} RivalMotionState;

void RivalMotionStep(RivalMotionState *state, s32 enableRaceBoost);

#endif
