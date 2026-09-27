#include "game/angle.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"

enum { PLAYER_BODY_BASE_OFFSET = 50 };

void CalculateCarBodyOffset(GameCarRuntime *car, s16 offset) {
    s16 pitchSin = WrapSigned16(SinAngle(car->bodyPitch));
    s16 pitchCos = WrapSigned16(CosAngle(car->bodyPitch));
    s16 yawSin = WrapSigned16(SinAngle(car->bodyYaw));
    s16 yawCos = WrapSigned16(CosAngle(car->bodyYaw));

    /* The old inverse (Rz * Rx * Ry) transform applied only a Z offset.
     * Its third column is the original rotation's third row; Rz leaves
     * that row unchanged. Keep the intermediate signed-16 truncations and
     * floor shifts from both matrix multiplications. */
    s16 x = WrapSigned16(((int64_t)pitchCos * yawSin) >> 12);
    s16 z = WrapSigned16(((int64_t)pitchCos * yawCos) >> 12);
    car->motionX = ((int64_t)x * offset) >> 12;
    car->motionY = ((int64_t)pitchSin * offset) >> 12;
    car->motionZ = ((int64_t)z * offset) >> 12;
}

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    const s16 offset = WrapSigned16(
        -(int64_t)car->drive.bodyLiftOffset - PLAYER_BODY_BASE_OFFSET);
    CalculateCarBodyOffset(AsRivalCar(car), offset);
}
