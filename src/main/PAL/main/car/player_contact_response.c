#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

static void PlayPlayerSkidCue(const PlayerCarRuntime *car, s32 skid,
                              s32 slip) {
    int lightTouch = skid == 1 || skid == 2;
    int nearSide = skid == 1 || skid == 3;

    if (skid < 1 || skid > 4 || (s16)car->motionTimer < 15) {
        return;
    }
    if ((u32)(slip - 768) < 257U) {
        if (lightTouch) {
            PlaySoundCue(0xA);
        } else if (car->speed >= 81) {
            PlaySoundCue(0xD);
        }
        return;
    }
    if (nearSide) {
        PlaySoundCue(g_MirrorMode == 0 ? 0xB : 0xC);
    } else {
        PlaySoundCue(g_MirrorMode == 0 ? 0xC : 0xB);
    }
}

void ApplyPlayerContactResponse(PlayerCarRuntime *car, s32 skid, s32 crash) {
    GameCarDrive *drive = &car->drive;
    s32 slip;
    s32 slipSin;
    s32 drivetrainScale;
    s32 speedScale;

    if (skid == 0 && crash == 0) {
        car->y += drive->standingStartBounceY;
        UpdateCarBodyKick(AsRivalCar(car));
        return;
    }

    slip = GetAngleDistance(
        0xC00 - TrackPoint(car->trackPointIndex)->angle,
        car->headingAngle);
    if (crash != 0) {
        drive->launchEnergy -= 1000;
        if (car->speed >= 81) {
            drive->drivetrainTorque = drive->drivetrainTorque * 98 / 100;
            car->speed = car->speed * 97 / 100;
            drive->engineLoad = drive->engineLoad * 95 / 100;
            g_ShiftTargetRpm = g_ShiftTargetRpm * 95 / 100;
        }
        return;
    }

    slipSin = rsin(slip);
    drivetrainScale = 85 - slipSin * 20 / 4096;
    speedScale = 87 - slipSin * 40 / 4096;
    drive->launchEnergy -= 5000;
    drive->drivetrainTorque =
        drivetrainScale * drive->drivetrainTorque / 100;
    car->speed = speedScale * car->speed / 100;
    drive->engineLoad = drive->engineLoad * drivetrainScale / 100;
    g_ShiftTargetRpm = drivetrainScale * g_ShiftTargetRpm / 100;
    if (g_RacePhase < 3) {
        PlayPlayerSkidCue(car, skid, slip);
    }
}
