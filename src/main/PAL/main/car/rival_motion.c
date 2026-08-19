#include "game/angle.h"
#include "game/rival_motion.h"

void RivalMotionStep(RivalMotionState *state,
                     const CarControlCommand *command) {
    if (command->boostEnabled && state->boostTimer > 0) {
        if (state->boostAccelerationThreshold < state->boostTimer &&
            state->speed >= 0x321) {
            state->acceleration = 0;
        } else if (state->boostAccelerationLimit >= state->acceleration) {
            state->acceleration += state->boostAcceleration;
        } else {
            state->acceleration = state->boostAccelerationLimit;
        }
        state->boostTimer--;
    } else if ((command->boostEnabled &&
                state->acceleration <= state->accelerationLimit) ||
               (!command->boostEnabled &&
                state->acceleration < state->accelerationLimit)) {
        state->acceleration += state->accelerationStep;
    } else {
        state->acceleration = state->accelerationLimit;
    }
    state->speed = state->speed * 94 / 100 + state->acceleration;
    state->bodyYaw += GetAngleDelta(state->bodyYaw, command->targetYaw) / 5;
}
