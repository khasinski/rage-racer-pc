#include "game/car.h"
#include "game/rival.h"
#include "game/car_control.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"

enum {
    COLLISION_SPEED_RETENTION_PERCENT = 97,
    PERCENT_SCALE = 100,
};

static void DampCollidingRivalSpeed(GameCarRuntime *car) {
    /* Retail applies two distinct 97% steps. Keep both divisions: folding
     * them into one percentage changes low-speed rounding. */
    car->speed = WrapSigned32(
        (int64_t)car->speed * COLLISION_SPEED_RETENTION_PERCENT) /
        PERCENT_SCALE;
    car->speed = WrapSigned32(
        (int64_t)car->speed * COLLISION_SPEED_RETENTION_PERCENT) /
        PERCENT_SCALE;
}

void FinishRival(GameCarRuntime *car, const TrackEventData *events,
                    s32 trackLength, int reverse) {
    if (car == NULL || car->activeFlag == -1) return;
    s32 ground;
    ground = WrapSigned32(
        (int64_t)car->y - CAR_WHEEL_GROUND_OFFSET);
    UpdateCarWheelRotation(car);
    CopyCarBodyRotationToModel(car);
    car->bodyRoll = WrapSigned32(
        (int64_t)car->bodyRoll + car->bodyRollVelocity);
    car->modelY = car->y;
    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        AdvanceCarJumpArc(car, ground);
        if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
            ApplyCarLandingPose(car, ground);
        }
    }
    if (car->collisionFlag == 0) {
        UpdateCarBodyKick(car);
        StepCarCrestHop(car, events, trackLength, reverse);
    } else {
        DampCollidingRivalSpeed(car);
    }
}
