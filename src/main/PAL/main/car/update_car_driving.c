#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/audio.h"

enum {
    DRIVING_YAW_RESPONSE = 5,
    PEDAL_INPUT_RELEASED = 128,
    REDLINE_EFFECT_RPM_MARGIN = 1000,
    REDLINE_EFFECT_HOLD_FRAMES = 41,
    REDLINE_EFFECT_MAX_LEVEL = 100,
};

static s32 UpdateDrivingVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 sidewaysMotion;
    s32 forwardMotion;
    s32 headingCorrection;

    headingCorrection = GetAngleDelta(car->bodyYaw, drive->targetHeading);
    car->bodyYaw += headingCorrection / DRIVING_YAW_RESPONSE;
    UpdateCarTravelVelocity(AsRivalCar(car));

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
    return sidewaysMotion;
}

static void UpdateDrivingRedlineEffect(const PlayerCarRuntime *car,
                                       const GameCarSpec *spec) {
    const GameCarDrive *drive = &car->drive;
    s32 voiceLevel;

    if (drive->engineRpm <= spec->redline + REDLINE_EFFECT_RPM_MARGIN ||
        g_SteerHoldFrames < REDLINE_EFFECT_HOLD_FRAMES ||
        drive->gear != spec->topGear ||
        car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    voiceLevel = g_SteerHoldFrames + 24;
    if (voiceLevel > REDLINE_EFFECT_MAX_LEVEL) {
        voiceLevel = REDLINE_EFFECT_MAX_LEVEL;
    }
    SetIndexedEffectVoice(2, 0x1500, voiceLevel);
}

static void BeginDrivingTakeoff(PlayerCarRuntime *car, s32 spinRate) {
    GameCarDrive *drive = &car->drive;

    drive->motionState = CAR_MOTION_TAKEOFF;
    drive->bodyLiftOffset = 0;
    drive->spinRate = spinRate;
    drive->launchDirection = car->facingBackwards;
    SetIndexedEffectVoice(0, 0, 0);
}

static void UpdateDrivingTakeoff(PlayerCarRuntime *car,
                                 const LaunchSpeedThreshold *launchThreshold,
                                 s32 sidewaysMotion) {
    GameCarDrive *drive = &car->drive;

    if (drive->acceleratorLatch == 1) {
        drive->launchEnergy = car->speed * drive->coastFrames;
        drive->coastFrames = 0;
        if (launchThreshold->initial < car->speed &&
            drive->launchEnergy > drive->launchEnergyThreshold) {
            s32 spinScale =
                1000 - (drive->steeringGripResponse - 1000) * 8;

            if (spinScale < 1000) {
                spinScale = 1000;
            }
            BeginDrivingTakeoff(
                car, -sidewaysMotion * spinScale / 1000 * 2);
        }
        return;
    }

    if (drive->acceleratorInput.value >= PEDAL_INPUT_RELEASED) {
        drive->coastFrames = 0;
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
            BeginDrivingTakeoff(car, -sidewaysMotion);
        }
        return;
    }

    drive->coastFrames++;
    drive->launchEnergy = 0;
}

void UpdateCarDriving(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const LaunchSpeedThreshold *launchThreshold =
        &g_LaunchSpeedThresholds[
            NormalizeCarLaunchThresholdIndex(drive->launchThresholdIndex)];
    s32 sidewaysMotion = UpdateDrivingVelocity(car);

    UpdateDrivingRedlineEffect(car, g_CarSpec);
    UpdateDrivingTakeoff(car, launchThreshold, sidewaysMotion);
}
