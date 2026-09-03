#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/render.h"

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    Matrix bodyRotation;
    Matrix rotationStep;
    Matrix inverseBodyRotation;
    SVec bodyOffset = {
        .vx = 0,
        .vy = 0,
        .vz = WrapSigned16(
            -(int64_t)car->drive.bodyLiftOffset - 50),
    };

    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&rotationStep, car->bodyPitch);
    MulMatrix2(&rotationStep, &bodyRotation);
    BuildRotMatrixZ(&rotationStep, car->bodyRoll);
    MulMatrix2(&rotationStep, &bodyRotation);
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &bodyOffset, &car->motionX);
}
