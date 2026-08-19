#ifndef GAME_RIVAL_MOTION_H
#define GAME_RIVAL_MOTION_H

#include "common.h"
#include "game/car_control_command.h"

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
} RivalMotionState;

void RivalMotionStep(RivalMotionState *state,
                     const CarControlCommand *command);

#endif
