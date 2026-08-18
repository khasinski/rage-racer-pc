#include "game/gearbox.h"

void GearboxUpdate(GearboxState *state, const GearboxInput *input) {
    if (state->manual != 0) {
        if (input->shiftUpPressed && state->gear < input->topGear &&
            state->clutch == 0) {
            state->gear++;
            state->steerHoldFrames = 0;
        }
        if (input->shiftDownPressed && state->gear >= 2) {
            state->gear--;
            state->steerHoldFrames = 0;
        }
        return;
    }
    if (input->shiftState == 0) {
        s32 index = state->gear - 1;
        if (input->speed < input->shiftPoints[index].downshiftSpeed &&
            state->autoShiftCooldown <= 0 && state->clutch == 0) {
            if (state->gear >= 2) {
                state->gear--;
                state->autoShiftCooldown = 25;
                state->steerHoldFrames = 0;
            }
        } else if (input->shiftPoints[index].upshiftSpeed < input->speed &&
                   state->autoShiftCooldown <= 0 && state->clutch == 0 &&
                   state->gear < input->topGear) {
            state->gear++;
            state->autoShiftCooldown = 25;
            state->steerHoldFrames = 0;
        }
    }
    if (state->autoShiftCooldown > 0)
        state->autoShiftCooldown -= state->brakeInput >= 129 ? 2 : 1;
    if (input->speed == 0 && state->gear >= 2 &&
        state->motionState != CAR_MOTION_STANDING_START) {
        state->gear = 1;
        state->clutch = 0;
        state->autoShiftCooldown = 0;
    }
}
