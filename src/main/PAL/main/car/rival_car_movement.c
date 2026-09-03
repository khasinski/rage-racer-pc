#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/render.h"

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
    Matrix bodyRotation;
    Matrix inverseBodyRotation;
    Matrix work;
    SVec lean;
    Vec4 transformedLean;
    s32 yawStep = car->yawRate;
    s32 yawLean = yawStep < 0
        ? (s32)(-(int64_t)yawStep / RIVAL_YAW_LEAN_DIVISOR)
        : yawStep / RIVAL_YAW_LEAN_DIVISOR;

    car->x = WrapSigned32((int64_t)car->x - car->motionX);
    car->z = WrapSigned32((int64_t)car->z - car->motionZ);
    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&work, car->bodyPitch);
    MulMatrix2(&work, &bodyRotation);
    BuildRotMatrixZ(&work, car->bodyRoll);
    MulMatrix2(&work, &bodyRotation);

    lean.vx = 0;
    lean.vy = 0;
    lean.vz = WrapSigned16(-(int64_t)yawLean - RIVAL_STATIC_BODY_LEAN);
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &lean, &transformedLean);
    car->motionX = transformedLean.x;
    car->motionY = transformedLean.y;
    car->motionZ = transformedLean.z;
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

    car->steeringAngle = WrapSigned32(
        (int64_t)car->steeringAngle + car->yawRate);
    if (car->steeringAngle > RIVAL_STEERING_LIMIT) {
        car->steeringAngle = RIVAL_STEERING_LIMIT;
    } else if (car->steeringAngle < -RIVAL_STEERING_LIMIT) {
        car->steeringAngle = -RIVAL_STEERING_LIMIT;
    }
    car->bodyYaw = WrapSigned32((int64_t)car->bodyYaw + car->yawRate);
}

void MoveRivalCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }

        car->baseBodyYaw = car->bodyYaw;
        car->worldVelocityX = WrapSigned32(
            (int64_t)rsin(car->headingAngle) * car->speed) /
            VELOCITY_COMPONENT_DIVISOR;
        car->worldVelocityZ = WrapSigned32(
            (int64_t)rcos(car->headingAngle) * car->speed) /
            VELOCITY_COMPONENT_DIVISOR;
        if (index < RIVAL_CONTENDER_COUNT) {
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
}
