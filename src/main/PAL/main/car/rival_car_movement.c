#include "game/car.h"
#include "game/integer.h"
#include "game/render.h"

enum {
    DETAILED_RIVAL_MOTION_COUNT = 4,
    RIVAL_STEERING_LIMIT = 0x12C,
};

static void ApplyDetailedBodyLean(GameCarRuntime *car) {
    Matrix bodyRotation;
    Matrix inverseBodyRotation;
    Matrix work;
    SVec lean;
    s32 yawStep = car->yawRate;
    s32 yawLean = yawStep < 0
        ? (s32)(-(int64_t)yawStep / 6)
        : yawStep / 6;

    car->x = WrapSigned32((int64_t)car->x - car->motionX);
    car->z = WrapSigned32((int64_t)car->z - car->motionZ);
    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&work, car->bodyPitch);
    MulMatrix2(&work, &bodyRotation);
    BuildRotMatrixZ(&work, car->bodyRoll);
    MulMatrix2(&work, &bodyRotation);

    lean.vx = 0;
    lean.vy = 0;
    lean.vz = WrapSigned16(-(int64_t)yawLean - 0x32);
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &lean, &car->motionX);
    car->x = WrapSigned32((int64_t)car->x + car->motionX);
    car->z = WrapSigned32((int64_t)car->z + car->motionZ);
}

static void UpdateRivalSteeringLean(GameCarRuntime *car) {
    if (car->steeringAngle >= 0x41) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity - 6);
    } else if (car->steeringAngle < -0x40) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity + 6);
    }
    if (car->bodyRollVelocity != 0) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity * 7) / 8;
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
            (int64_t)rsin(car->headingAngle) * car->speed) / 256;
        car->worldVelocityZ = WrapSigned32(
            (int64_t)rcos(car->headingAngle) * car->speed) / 256;
        if (index < DETAILED_RIVAL_MOTION_COUNT) {
            ApplyDetailedBodyLean(car);
        }
        car->x = WrapSigned32(
            (int64_t)car->x +
            WrapSigned32((int64_t)car->worldVelocityX * 6) / 1280);
        car->z = WrapSigned32(
            (int64_t)car->z +
            WrapSigned32((int64_t)car->worldVelocityZ * 6) / 1280);
        UpdateRivalSteeringLean(car);
    }
}
