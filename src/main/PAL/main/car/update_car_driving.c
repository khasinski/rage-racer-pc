#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/audio.h"

/*
 * Car motion handler for motionState == CAR_MOTION_DRIVING (normal driving): turns steering into a
 * world velocity, triggers over-rev / redline engine-audio cues (comparing
 * against the spec block's redline at +0x100 / +0x106), advances the car
 * (AdvanceCarPosition), and detects the jump/launch trigger. The drive sub-block is
 * the GameCarDrive view beginning at offset +0xBC.
 */
void UpdateCarDriving(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    const LaunchSpeedThreshold *launchThreshold =
        &g_LaunchSpeedThresholds[
            NormalizeCarLaunchThresholdIndex(drive->launchThresholdIndex)];
    s32 bodySin;
    s32 bodyCos;
    s32 sidewaysMotion;
    s32 forwardMotion;
    s32 headingCorrection;

    headingCorrection = GetAngleDelta(car->bodyYaw, drive->targetHeading);
    car->bodyYaw += headingCorrection / 5;
    AdvanceCarPosition(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);

    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;

    sidewaysMotion =
        (bodyCos * drive->accelPos - bodySin * drive->brakePos) / 4096;
    forwardMotion =
        (bodySin * drive->accelPos + bodyCos * drive->brakePos) / 4096;
    drive->accelPos = bodySin * forwardMotion / 4096;
    drive->brakePos = bodyCos * forwardMotion / 4096;

    if (spec->revLimit + 2000 < drive->engineRpm && g_RacePhase >= 2) {
        SetIndexedEffectVoice(0, 0x1800,
                              (drive->engineRpm - spec->revLimit) / 100 + 128);
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (spec->redline + 1000 < drive->engineRpm) {
        s16 holdFrames = g_SteerHoldFrames;

        if (holdFrames >= 41 && drive->gear == spec->topGear &&
            car->verticalMotionState == 0) {
            s32 voiceLevel = holdFrames + 24;

            if (voiceLevel >= 101) {
                voiceLevel = 100;
            }
            SetIndexedEffectVoice(2, 0x1500, voiceLevel);
        } else {
            SetIndexedEffectVoice(-1, 0, 0);
        }
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (drive->acceleratorLatch == 1) {
        drive->launchEnergy = car->speed * drive->groundedFrames;
        drive->groundedFrames = 0;
        if (launchThreshold->initial < car->speed &&
            drive->launchEnergy > drive->launchEnergyThreshold) {
            s32 spinScale =
                1000 - (drive->steeringGripResponse - 1000) * 8;

            drive->motionState = CAR_MOTION_TAKEOFF;
            drive->bodyLiftOffset = 0;
            SetIndexedEffectVoice(0, 0, 0);
            if (spinScale < 1000) {
                spinScale = 1000;
            }
            drive->spinRate = -sidewaysMotion * spinScale / 1000 * 2;
            drive->launchDirection = car->facingBackwards;
        }
        return;
    }

    if (drive->acceleratorInput.value >= 128) {
        drive->groundedFrames = 0;
        drive->launchEnergy = 0;
        return;
    }

    if (drive->brakeLatch == 1) {
        s32 sidewaysSpeed =
            (sidewaysMotion < 0 ? -sidewaysMotion : sidewaysMotion) *
            car->speed / 64;

        drive->launchEnergy = sidewaysSpeed;
        if (launchThreshold->sustain < car->speed &&
            drive->launchEnergyThreshold < sidewaysSpeed) {
            drive->motionState = CAR_MOTION_TAKEOFF;
            drive->bodyLiftOffset = 0;
            SetIndexedEffectVoice(0, 0, 0);
            drive->spinRate = -sidewaysMotion;
            drive->launchDirection = car->facingBackwards;
        }
        return;
    }

    drive->groundedFrames++;
    drive->launchEnergy = 0;
}
