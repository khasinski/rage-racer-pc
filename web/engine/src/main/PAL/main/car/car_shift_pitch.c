#include "game/car_drive.h"
#include "game/integer.h"
#include "game/random.h"

void StepCarShiftPitch(PlayerCarRuntime *car, const GameCarSpec *spec, u32 *random) {
    if (car->drive.shiftRpmDelta == 0) return;
    const s32 surplus = WrapSigned32(
        ((int64_t)spec->revLimit + spec->redline) / 2 - car->drive.shiftTargetRpm);
    if (surplus > 0) {
        const s32 kick = WrapSigned32((int64_t)surplus * RandomNext(random)) /
                         (100 * 0x7FFF);
        car->bodyPitch = WrapSigned32((int64_t)car->bodyPitch + kick);
    }
}
