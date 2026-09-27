#ifndef GAME_CAR_MOTION_INTERNAL_H
#define GAME_CAR_MOTION_INTERNAL_H

#include "game/car.h"

struct GameTrackPoint;
struct TrackEventData;
s32 FindCarCrest(const GameCarRuntime *car, const struct TrackEventData *data,
                 s32 trackLength, int reverse);
void StepCarCrestHop(GameCarRuntime *car, const struct TrackEventData *data,
                     s32 trackLength, int reverse);
/* Returns the skid angle, or -1 when there was no skid response. */
s32 ApplyCarContactResponse(PlayerCarRuntime *car,
                             const struct GameTrackPoint *point,
                             s32 skid, s32 crash);
enum { CAR_CONTACT_MIN_SPEED = 81 };

enum {
    CAR_WHEEL_GROUND_OFFSET = 8,
    CAR_JUMP_RISE_CURVE = 72,
    CAR_JUMP_FALL_CURVE = 216,
    CAR_JUMP_CURVE_SCALE = 100,
};

void AdvanceCarJumpArc(GameCarRuntime *car, s32 groundHeight);
/* Returns nonzero only on a landing, so the client can play its cue. */
int StepPlayerJump(PlayerCarRuntime *car, const GameCarSpec *spec,
                    s32 groundHeight);

enum {
    CAR_BODY_KICK_DURATION = 30,
};

void IntegratePlayerPosition(PlayerCarRuntime *car);
void CalculateCarBodyOffset(GameCarRuntime *car, s16 offset);
void CalculatePlayerBodyOffset(PlayerCarRuntime *car);
void ApplyCarKnockback(GameCarRuntime *car);
void ApplyCarLandingPose(GameCarRuntime *car, s32 groundHeight);
void SetCarCollisionKnockback(GameCarRuntime *car, s32 x, s32 z);
void SetTrackBoundaryKnockback(GameCarRuntime *car, s32 x, s32 z,
                               CarTrackContact contact);
/* Return the course crest crossed by this car during the current frame. */
s32 GetCarCrestTrigger(const GameCarRuntime *car);
/* Track heading and random sample belong to the caller's race. */
void BeginCarBodyKick(GameCarRuntime *car, CarBodyKickMode mode,
                       s32 trackHeading, s32 random);
void UpdateCarBodyKick(GameCarRuntime *car);
void UpdateCarBodyRoll(PlayerCarRuntime *car);
void UpdateCarCrestHop(GameCarRuntime *car);
void StepCarSlide(GameCarRuntime *car, s32 slideScale, int reverse);
void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale);
void UpdateCarTilt(PlayerCarRuntime *car, const GameCarSpec *spec, int racing);

#endif
