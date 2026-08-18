#ifndef GAME_GEARBOX_H
#define GAME_GEARBOX_H

#include "common.h"
#include "game/car.h"

typedef struct GearboxState {
    s16 gear, clutch, manual, brakeInput, motionState;
    s32 autoShiftCooldown;
    s32 steerHoldFrames;
} GearboxState;

typedef struct GearboxInput {
    s32 speed, shiftState, shiftUpPressed, shiftDownPressed;
    s16 topGear;
    const GameCarSpecShiftPoint *shiftPoints;
} GearboxInput;

void GearboxUpdate(GearboxState *state, const GearboxInput *input);

#endif
