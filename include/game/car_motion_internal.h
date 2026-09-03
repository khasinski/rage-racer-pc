#ifndef GAME_CAR_MOTION_INTERNAL_H
#define GAME_CAR_MOTION_INTERNAL_H

#include "game/car.h"

void ApplyCarKnockback(GameCarRuntime *car);
void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode);
void StartCarBodyKick(GameCarRuntime *car, s32 mode);
void UpdateCarBodyKick(GameCarRuntime *car);
void UpdateCarBodyRoll(PlayerCarRuntime *car);
void UpdateCarCrestHop(GameCarRuntime *car);
void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale);
void UpdatePlayerTilt(PlayerCarRuntime *car);

#endif
