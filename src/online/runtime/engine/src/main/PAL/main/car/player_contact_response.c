#include "game/angle.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/track.h"

enum {
    PERCENT_SCALE = 100,
    TRIG_SCALE = 4096,
    CRASH_LAUNCH_ENERGY_LOSS = 1000,
    CRASH_TORQUE_RETENTION_PERCENT = 98,
    CRASH_SPEED_RETENTION_PERCENT = 97,
    CRASH_ENGINE_RETENTION_PERCENT = 95,
    SKID_LAUNCH_ENERGY_LOSS = 5000,
    SKID_TORQUE_BASE_PERCENT = 85,
    SKID_TORQUE_SLIP_PERCENT = 20,
    SKID_SPEED_BASE_PERCENT = 87,
    SKID_SPEED_SLIP_PERCENT = 40,
};

static void ApplyPlayerCrashResponse(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - CRASH_LAUNCH_ENERGY_LOSS);
    if (car->speed < CAR_CONTACT_MIN_SPEED) {
        return;
    }

    drive->drivetrainTorque = WrapSigned32(
        (int64_t)drive->drivetrainTorque *
        CRASH_TORQUE_RETENTION_PERCENT) / PERCENT_SCALE;
    car->speed = WrapSigned32(
        (int64_t)car->speed * CRASH_SPEED_RETENTION_PERCENT) / PERCENT_SCALE;
    drive->engineLoad = WrapSigned16(
        (int64_t)drive->engineLoad * CRASH_ENGINE_RETENTION_PERCENT /
        PERCENT_SCALE);
    drive->shiftTargetRpm = WrapSigned32(
        (int64_t)drive->shiftTargetRpm * CRASH_ENGINE_RETENTION_PERCENT) /
        PERCENT_SCALE;
}

static s32 ApplyPlayerSkidResponse(PlayerCarRuntime *car,
                                    const GameTrackPoint *point) {
    GameCarDrive *drive = &car->drive;
    s32 slip;
    s32 slipSin;
    s32 drivetrainScale;
    s32 speedScale;

    if (point == NULL) {
        return -1;
    }
    slip = GetAngleDistance(
        ANGLE_THREE_QUARTER_TURN -
            point->angle,
        car->headingAngle);

    slipSin = SinAngle(slip);
    drivetrainScale =
        SKID_TORQUE_BASE_PERCENT -
        slipSin * SKID_TORQUE_SLIP_PERCENT / TRIG_SCALE;
    speedScale =
        SKID_SPEED_BASE_PERCENT -
        slipSin * SKID_SPEED_SLIP_PERCENT / TRIG_SCALE;
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - SKID_LAUNCH_ENERGY_LOSS);
    drive->drivetrainTorque = WrapSigned32(
        (int64_t)drivetrainScale * drive->drivetrainTorque) / PERCENT_SCALE;
    car->speed = WrapSigned32(
        (int64_t)speedScale * car->speed) / PERCENT_SCALE;
    drive->engineLoad = WrapSigned16(
        (int64_t)drive->engineLoad * drivetrainScale / PERCENT_SCALE);
    drive->shiftTargetRpm = WrapSigned32(
        (int64_t)drivetrainScale * drive->shiftTargetRpm) / PERCENT_SCALE;
    return slip;
}

s32 ApplyCarContactResponse(PlayerCarRuntime *car, const GameTrackPoint *point,
                             s32 skid, s32 crash) {
    if (skid == 0 && crash == 0) {
        car->y = WrapSigned32(
            (int64_t)car->y + car->drive.standingStartBounceY);
        UpdateCarBodyKick(AsRivalCar(car));
    } else if (crash != 0) {
        ApplyPlayerCrashResponse(car);
    } else {
        return ApplyPlayerSkidResponse(car, point);
    }
    return -1;
}
