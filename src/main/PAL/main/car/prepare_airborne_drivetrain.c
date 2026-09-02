#include "game/car.h"
#include "game/car_internal.h"

enum {
    AIRBORNE_SHIFT_FRAMES = 20,
    AUTOMATIC_ENGINE_LOAD_SCALE = 985,
    ENGINE_LOAD_SCALE = 1000,
};

void PrepareAirborneDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    s32 gearRatio = GetPositiveCarGearRatio(spec, drive->gear);
    s32 rpm;

    drive->drivetrainTorque =
        (100 - (drive->gear - 1) * 4) * 10000 * car->speed / 100;
    rpm = car->speed * 160 / 1168 * 10000 / gearRatio;

    drive->jumpTimer = AIRBORNE_SHIFT_FRAMES;
    drive->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = rpm;
    drive->shiftRpmDelta = (s16)((u16)rpm - (u16)drive->engineRpm);
    drive->engineLoad = rpm * GetCarGearLoad(spec, drive->gear) / 0x20000;
    if (drive->manual == 0) {
        drive->engineLoad =
            drive->engineLoad * AUTOMATIC_ENGINE_LOAD_SCALE /
            ENGINE_LOAD_SCALE;
    }
}
