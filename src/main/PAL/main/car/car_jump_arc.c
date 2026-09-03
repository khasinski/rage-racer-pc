#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

static s32 JumpCurve(s32 tick, s32 scale) {
    s32 squared = WrapSigned32((int64_t)tick * tick);

    return WrapSigned32((int64_t)squared * scale) /
           CAR_JUMP_CURVE_SCALE;
}

void ApplyCarLandingPose(GameCarRuntime *car, s32 groundHeight) {
    car->y = WrapSigned32(
        (int64_t)groundHeight + CAR_WHEEL_GROUND_OFFSET);
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(car, CAR_BODY_KICK_LANDING);
}

static void AdvanceRisingJump(GameCarRuntime *car, s16 tick,
                              s32 groundHeight) {
    s32 velocity = WrapSigned32(
        (int64_t)car->verticalMotionRate * tick);

    car->y = WrapSigned32((int64_t)car->y + velocity);
    car->y = WrapSigned32(
        (int64_t)car->y + JumpCurve(tick, CAR_JUMP_RISE_CURVE));
    if (car->y >= groundHeight) {
        car->verticalMotionState = CAR_VERTICAL_GROUNDED;
    }
}

static void HoldJumpAtCrest(GameCarRuntime *car, s16 tick,
                            s32 groundHeight) {
    car->y = car->verticalTargetY;
    if (WrapSigned32((int64_t)groundHeight - car->verticalMotionRate) >
        car->verticalTargetY) {
        car->verticalMotionState = CAR_VERTICAL_FALLING;
        car->verticalMotionRate = tick;
    }
}

static void AdvanceFallingJump(GameCarRuntime *car, s16 tick,
                               s32 groundHeight) {
    s16 fall = WrapSigned16((s32)tick - car->verticalMotionRate);

    car->y = WrapSigned32(
        (int64_t)car->verticalTargetY +
        JumpCurve(fall, CAR_JUMP_FALL_CURVE));
    if (car->y >= groundHeight) {
        car->verticalMotionState = CAR_VERTICAL_GROUNDED;
    }
}

/* Advance the shared airborne parabola. Landing side effects belong to the
 * player and rival callers, which deliberately handle them differently. */
void AdvanceCarJumpArc(GameCarRuntime *car, s32 groundHeight) {
    s16 tick = WrapSigned16((s32)car->verticalMotionTimer + 1);

    car->verticalMotionTimer = tick;
    switch (car->verticalMotionState) {
    case CAR_VERTICAL_RISING:
        AdvanceRisingJump(car, tick, groundHeight);
        break;
    case CAR_VERTICAL_AT_CREST:
        HoldJumpAtCrest(car, tick, groundHeight);
        break;
    case CAR_VERTICAL_FALLING:
    default:
        AdvanceFallingJump(car, tick, groundHeight);
        break;
    }
}
