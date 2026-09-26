#include "game/car.h"
#include "game/angle.h"
#include "game/rival.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"

enum {
    RIVAL_STEERING_DEAD_ZONE = 0x40,
    RIVAL_STEERING_LIMIT = 0x12C,
    RIVAL_BODY_ROLL_ACCELERATION = 6,
    RIVAL_YAW_LEAN_DIVISOR = 6,
    RIVAL_STATIC_BODY_LEAN = 0x32,
    RIVAL_POSITION_STEP_NUMERATOR = 6,
    RIVAL_POSITION_STEP_DIVISOR = 1280,
    BODY_ROLL_DAMPING_NUMERATOR = 7,
    BODY_ROLL_DAMPING_DENOMINATOR = 8,
    VELOCITY_COMPONENT_DIVISOR = 256,
};

static void ApplyDetailedBodyLean(GameCarRuntime *car) {
    s32 yawStep = car->yawRate;
    s32 yawLean = yawStep < 0
        ? (s32)(-(int64_t)yawStep / RIVAL_YAW_LEAN_DIVISOR)
        : yawStep / RIVAL_YAW_LEAN_DIVISOR;

    car->x = WrapSigned32((int64_t)car->x - car->motionX);
    car->z = WrapSigned32((int64_t)car->z - car->motionZ);
    CalculateCarBodyOffset(car, WrapSigned16(-(int64_t)yawLean - RIVAL_STATIC_BODY_LEAN));
    car->x = WrapSigned32((int64_t)car->x + car->motionX);
    car->z = WrapSigned32((int64_t)car->z + car->motionZ);
}

static void UpdateRivalSteeringLean(GameCarRuntime *car) {
    if (car->steeringAngle > RIVAL_STEERING_DEAD_ZONE) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity - RIVAL_BODY_ROLL_ACCELERATION);
    } else if (car->steeringAngle < -RIVAL_STEERING_DEAD_ZONE) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity + RIVAL_BODY_ROLL_ACCELERATION);
    }
    if (car->bodyRollVelocity != 0) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity * BODY_ROLL_DAMPING_NUMERATOR) /
            BODY_ROLL_DAMPING_DENOMINATOR;
    }
    car->steeringAngle = WrapSigned32((int64_t)car->steeringAngle + car->yawRate);
    if (car->steeringAngle > RIVAL_STEERING_LIMIT) {
        car->steeringAngle = RIVAL_STEERING_LIMIT;
    } else if (car->steeringAngle < -RIVAL_STEERING_LIMIT) {
        car->steeringAngle = -RIVAL_STEERING_LIMIT;
    }
    car->bodyYaw = WrapSigned32((int64_t)car->bodyYaw + car->yawRate);
}

void MoveRival(GameCarRuntime *car, s32 slot) {
    if (car == NULL || car->activeFlag == -1 || (u32)slot >= RACE_CAR_SLOT_COUNT) return;
    car->baseBodyYaw = car->bodyYaw;
    car->worldVelocityX = WrapSigned32(
        (int64_t)SinAngle(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;
    car->worldVelocityZ = WrapSigned32(
        (int64_t)CosAngle(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;
    if (slot < RIVAL_CONTENDER_COUNT) {
        ApplyDetailedBodyLean(car);
    }
    car->x = WrapSigned32(
        (int64_t)car->x +
        WrapSigned32((int64_t)car->worldVelocityX *
                     RIVAL_POSITION_STEP_NUMERATOR) /
            RIVAL_POSITION_STEP_DIVISOR);
    car->z = WrapSigned32(
        (int64_t)car->z +
        WrapSigned32((int64_t)car->worldVelocityZ *
                     RIVAL_POSITION_STEP_NUMERATOR) /
            RIVAL_POSITION_STEP_DIVISOR);
    UpdateRivalSteeringLean(car);
}
