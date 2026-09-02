#include "game/car.h"
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
    s32 yawLean = yawStep < 0 ? -yawStep / 6 : yawStep / 6;

    car->x -= car->motionX;
    car->z -= car->motionZ;
    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&work, car->bodyPitch);
    MulMatrix2(&work, &bodyRotation);
    BuildRotMatrixZ(&work, car->bodyRoll);
    MulMatrix2(&work, &bodyRotation);

    lean.vx = 0;
    lean.vy = 0;
    lean.vz = -yawLean - 0x32;
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &lean, &car->motionX);
    car->x += car->motionX;
    car->z += car->motionZ;
}

static void UpdateRivalSteeringLean(GameCarRuntime *car) {
    if (car->steeringAngle >= 0x41) {
        car->bodyRollVelocity -= 6;
    } else if (car->steeringAngle < -0x40) {
        car->bodyRollVelocity += 6;
    }
    if (car->bodyRollVelocity != 0) {
        car->bodyRollVelocity = car->bodyRollVelocity * 7 / 8;
    }

    car->steeringAngle += car->yawRate;
    if (car->steeringAngle > RIVAL_STEERING_LIMIT) {
        car->steeringAngle = RIVAL_STEERING_LIMIT;
    } else if (car->steeringAngle < -RIVAL_STEERING_LIMIT) {
        car->steeringAngle = -RIVAL_STEERING_LIMIT;
    }
    car->bodyYaw += car->yawRate;
}

void MoveRivalCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }

        car->baseBodyYaw = car->bodyYaw;
        car->worldVelocityX = rsin(car->headingAngle) * car->speed / 256;
        car->worldVelocityZ = rcos(car->headingAngle) * car->speed / 256;
        if (index < DETAILED_RIVAL_MOTION_COUNT) {
            ApplyDetailedBodyLean(car);
        }
        car->x += car->worldVelocityX * 6 / 1280;
        car->z += car->worldVelocityZ * 6 / 1280;
        UpdateRivalSteeringLean(car);
    }
}
