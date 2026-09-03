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
    int lightTouch = skid == 1 || skid == 2;
    int nearSide = skid == 1 || skid == 3;

    if (skid < 1 || skid > 4 ||
        (s16)car->motionTimer < SKID_CUE_MIN_TIMER) {
        return;
    }
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
        (int64_t)drive->launchEnergy - 1000);
    if (car->speed < CONTACT_EFFECT_MIN_SPEED) return;

    drive->drivetrainTorque = WrapSigned32(
        (int64_t)drive->drivetrainTorque * 98) / 100;
    car->speed = WrapSigned32((int64_t)car->speed * 97) / 100;
    drive->engineLoad = WrapSigned16(
        (int64_t)drive->engineLoad * 95 / 100);
    g_ShiftTargetRpm = WrapSigned32(
        (int64_t)g_ShiftTargetRpm * 95) / 100;
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
    drivetrainScale = 85 - slipSin * 20 / 4096;
    speedScale = 87 - slipSin * 40 / 4096;
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - 5000);
    drive->drivetrainTorque = WrapSigned32(
        (int64_t)drivetrainScale * drive->drivetrainTorque) / 100;
    car->speed = WrapSigned32((int64_t)speedScale * car->speed) / 100;
    drive->engineLoad = WrapSigned16(
        (int64_t)drive->engineLoad * drivetrainScale / 100);
    g_ShiftTargetRpm = WrapSigned32(
        (int64_t)drivetrainScale * g_ShiftTargetRpm) / 100;
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
