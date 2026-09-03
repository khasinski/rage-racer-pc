#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

enum {
    AUTOMATIC_ENGINE_LOAD_SCALE = 985,
    ENGINE_LOAD_SCALE = 1000,
};

void PrepareAirborneDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    s32 speedScale;
    s32 rpm;

    speedScale = WrapSigned32(
        (int64_t)100 - WrapSigned32((int64_t)(drive->gear - 1) * 4));
    speedScale = WrapSigned32((int64_t)speedScale * 10000);
    drive->drivetrainTorque =
        WrapSigned32((int64_t)speedScale * car->speed) / 100;
    rpm = CalculateAirborneEngineRpm(spec, drive->gear, car->speed);

    drive->jumpTimer = CAR_AIRBORNE_SHIFT_FRAMES;
    drive->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = rpm;
    drive->shiftRpmDelta = CalculateCarRpmDelta(rpm, drive->engineRpm);
    drive->engineLoad = WrapSigned16(
        WrapSigned32((int64_t)rpm *
                     GetCarGearLoad(spec, drive->gear)) /
        0x20000);
    if (drive->manual == 0) {
        drive->engineLoad = WrapSigned16(
            (int64_t)drive->engineLoad * AUTOMATIC_ENGINE_LOAD_SCALE /
            ENGINE_LOAD_SCALE);
    }
}
