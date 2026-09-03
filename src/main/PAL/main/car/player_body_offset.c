#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/render.h"

enum {
    PLAYER_BODY_BASE_OFFSET = 50,
};

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    Matrix bodyRotation;
    Matrix rotationStep;
    Matrix inverseBodyRotation;
    Vec4 transformedOffset;
    SVec bodyOffset = {
        .vx = 0,
        .vy = 0,
        .vz = WrapSigned16(
            -(int64_t)car->drive.bodyLiftOffset - PLAYER_BODY_BASE_OFFSET),
    };

    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&rotationStep, car->bodyPitch);
    MulMatrix2(&rotationStep, &bodyRotation);
    BuildRotMatrixZ(&rotationStep, car->bodyRoll);
    MulMatrix2(&rotationStep, &bodyRotation);
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &bodyOffset, &transformedOffset);
    car->motionX = transformedOffset.x;
    car->motionY = transformedOffset.y;
    car->motionZ = transformedOffset.z;
}
