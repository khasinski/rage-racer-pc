#ifndef GAME_CAR_PHYSICS_H
#define GAME_CAR_PHYSICS_H

#include "common.h"

s16 CarUpdatePedalLatch(s16 latch, s32 input);
s32 CarCalculateGripBudget(s32 acceleratorInput, s32 brakeInput);

#endif
