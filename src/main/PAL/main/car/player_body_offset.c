#include "game/car.h"
#include "game/car_internal.h"
#include "game/render.h"

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    Matrix bodyRotation;
    Matrix inverseBodyRotation;
    SVec bodyOffset = {
        .vx = 0,
        .vy = 0,
        .vz = (s16)(-car->drive.bodyLiftOffset - 50),
    };

    BuildRotMatrixY(&bodyRotation, car->bodyYaw);
    BuildRotMatrixX(&inverseBodyRotation, car->bodyPitch);
    MulMatrix2(&inverseBodyRotation, &bodyRotation);
    BuildRotMatrixZ(&inverseBodyRotation, car->bodyRoll);
    MulMatrix2(&inverseBodyRotation, &bodyRotation);
    TransposeMatrix(&bodyRotation, &inverseBodyRotation);
    ApplyMatrix(&inverseBodyRotation, &bodyOffset, &car->motionX);
}
