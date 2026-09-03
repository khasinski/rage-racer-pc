#ifndef GAME_CAR_MOTION_INTERNAL_H
#define GAME_CAR_MOTION_INTERNAL_H

#include "game/car.h"

enum {
    CAR_BODY_KICK_DURATION = 30,
};

void ApplyCarKnockback(GameCarRuntime *car);
void ApplyCarLandingPose(GameCarRuntime *car, s32 groundHeight);
void SetCarCollisionKnockback(GameCarRuntime *car, s32 x, s32 z);
void SetTrackBoundaryKnockback(GameCarRuntime *car, s32 x, s32 z,
                               CarTrackContact contact);
/* Return the course crest crossed by this car during the current frame. */
s32 GetCarCrestTrigger(const GameCarRuntime *car);
void StartCarBodyKick(GameCarRuntime *car, s32 mode);
void UpdateCarBodyKick(GameCarRuntime *car);
void UpdateCarBodyRoll(PlayerCarRuntime *car);
void UpdateCarCrestHop(GameCarRuntime *car);
void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale);
void UpdatePlayerTilt(PlayerCarRuntime *car);

#endif
