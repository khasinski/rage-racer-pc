#include "game/audio.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

enum {
    CONTACT_EFFECT_MIN_SPEED = 81,
    SKID_CUE_MIN_TIMER = 15,
    STRAIGHT_SLIP_MIN = 768,
    STRAIGHT_SLIP_SPAN = 257,
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
    CUE_LIGHT_SKID = 0xA,
    CUE_NEAR_SIDE_SKID = 0xB,
    CUE_FAR_SIDE_SKID = 0xC,
    CUE_HEAVY_SKID = 0xD,
};

static int IsStraightSlip(s32 slip) {
    return slip >= STRAIGHT_SLIP_MIN &&
           slip < STRAIGHT_SLIP_MIN + STRAIGHT_SLIP_SPAN;
}

static void PlayPlayerSkidCue(const PlayerCarRuntime *car, s32 skid,
                              s32 slip) {
    int lightTouch;
    int nearSide;

    if (skid < CAR_TRACK_CONTACT_FRONT_LEFT ||
        skid > CAR_TRACK_CONTACT_REAR_RIGHT ||
        WrapSigned16(car->motionTimer) < SKID_CUE_MIN_TIMER) {
        return;
    }
    lightTouch = skid <= CAR_TRACK_CONTACT_FRONT_RIGHT;
    nearSide = skid == CAR_TRACK_CONTACT_FRONT_LEFT ||
               skid == CAR_TRACK_CONTACT_REAR_LEFT;
    if (IsStraightSlip(slip)) {
        if (lightTouch) {
            PlaySoundCue(CUE_LIGHT_SKID);
        } else if (car->speed >= CONTACT_EFFECT_MIN_SPEED) {
            PlaySoundCue(CUE_HEAVY_SKID);
        }
        return;
    }
    if (nearSide) {
        PlaySoundCue(g_MirrorMode == 0 ? CUE_NEAR_SIDE_SKID
                                      : CUE_FAR_SIDE_SKID);
    } else {
        PlaySoundCue(g_MirrorMode == 0 ? CUE_FAR_SIDE_SKID
                                      : CUE_NEAR_SIDE_SKID);
    }
}

static void ApplyPlayerCrashResponse(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - CRASH_LAUNCH_ENERGY_LOSS);
    if (car->speed < CONTACT_EFFECT_MIN_SPEED) return;

    drive->drivetrainTorque = WrapSigned32(
        (int64_t)drive->drivetrainTorque *
        CRASH_TORQUE_RETENTION_PERCENT) / PERCENT_SCALE;
    car->speed = WrapSigned32(
        (int64_t)car->speed * CRASH_SPEED_RETENTION_PERCENT) / PERCENT_SCALE;
    drive->engineLoad = WrapSigned16(
        (int64_t)drive->engineLoad * CRASH_ENGINE_RETENTION_PERCENT /
        PERCENT_SCALE);
    g_ShiftTargetRpm = WrapSigned32(
        (int64_t)g_ShiftTargetRpm * CRASH_ENGINE_RETENTION_PERCENT) /
        PERCENT_SCALE;
}

static void ApplyPlayerSkidResponse(PlayerCarRuntime *car, s32 skid) {
    GameCarDrive *drive = &car->drive;
    s32 slip;
    s32 slipSin;
    s32 drivetrainScale;
    s32 speedScale;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return;
    }
    slip = GetAngleDistance(
        ANGLE_THREE_QUARTER_TURN -
            TrackPoint(car->trackPointIndex)->angle,
        car->headingAngle);

    slipSin = rsin(slip);
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
    g_ShiftTargetRpm = WrapSigned32(
        (int64_t)drivetrainScale * g_ShiftTargetRpm) / PERCENT_SCALE;
    if (g_RacePhase < 3) {
        PlayPlayerSkidCue(car, skid, slip);
    }
}

void ApplyPlayerContactResponse(PlayerCarRuntime *car, s32 skid, s32 crash) {
    if (skid == 0 && crash == 0) {
        car->y = WrapSigned32(
            (int64_t)car->y + car->drive.standingStartBounceY);
        UpdateCarBodyKick(AsRivalCar(car));
    } else if (crash != 0) {
        ApplyPlayerCrashResponse(car);
    } else {
        ApplyPlayerSkidResponse(car, skid);
    }
}
