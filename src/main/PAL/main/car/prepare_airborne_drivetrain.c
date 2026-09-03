#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

enum {
    AIRBORNE_BASE_SPEED_PERCENT = 100,
    AIRBORNE_GEAR_SPEED_PENALTY = 4,
    AIRBORNE_TORQUE_SCALE = 10000,
    PERCENT_SCALE = 100,
    ENGINE_LOAD_DIVISOR = 0x20000,
    AUTOMATIC_ENGINE_LOAD_SCALE = 985,
    ENGINE_LOAD_SCALE = 1000,
};

void PrepareAirborneDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    s32 speedScale;
    s32 rpm;

    speedScale = WrapSigned32(
        (int64_t)AIRBORNE_BASE_SPEED_PERCENT -
        WrapSigned32((int64_t)(drive->gear - 1) *
                     AIRBORNE_GEAR_SPEED_PENALTY));
    speedScale = WrapSigned32(
        (int64_t)speedScale * AIRBORNE_TORQUE_SCALE);
    drive->drivetrainTorque =
        WrapSigned32((int64_t)speedScale * car->speed) / PERCENT_SCALE;
    rpm = CalculateAirborneEngineRpm(spec, drive->gear, car->speed);

    drive->jumpTimer = CAR_AIRBORNE_SHIFT_FRAMES;
    drive->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = rpm;
    drive->shiftRpmDelta = CalculateCarRpmDelta(rpm, drive->engineRpm);
    drive->engineLoad = WrapSigned16(
        WrapSigned32((int64_t)rpm *
                     GetCarGearLoad(spec, drive->gear)) /
        ENGINE_LOAD_DIVISOR);
    if (drive->manual == 0) {
        drive->engineLoad = WrapSigned16(
            (int64_t)drive->engineLoad * AUTOMATIC_ENGINE_LOAD_SCALE /
            ENGINE_LOAD_SCALE);
    }
}
