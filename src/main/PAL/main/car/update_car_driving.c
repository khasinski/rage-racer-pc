#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/audio.h"

enum {
    DRIVING_YAW_RESPONSE = 5,
    PEDAL_INPUT_RELEASED = 128,
    REDLINE_EFFECT_RPM_MARGIN = 1000,
    REDLINE_EFFECT_HOLD_FRAMES = 41,
    REDLINE_EFFECT_MAX_LEVEL = 100,
    VELOCITY_COMPONENT_DIVISOR = 256,
    FIXED_TRIG_SCALE = 4096,
    REDLINE_EFFECT_VOLUME_BASE = 24,
    REDLINE_EFFECT_VOICE = 2,
    REDLINE_EFFECT_PHASE = 0x1500,
    STEERING_GRIP_BASE = 1000,
    STEERING_GRIP_SPIN_SCALE = 8,
    TAKEOFF_SPIN_MULTIPLIER = 2,
    BRAKING_SIDEWAYS_SPEED_DIVISOR = 64,
};

static s32 UpdateDrivingVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 sidewaysMotion;
    s32 forwardMotion;
    s32 headingCorrection;

    headingCorrection = GetAngleDelta(car->bodyYaw, drive->targetHeading);
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        headingCorrection / DRIVING_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);

    drive->accelPos = WrapSigned32(
        (int64_t)rsin(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)rcos(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;

    sidewaysMotion = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodyCos * drive->accelPos) -
        WrapSigned32((int64_t)bodySin * drive->brakePos)) /
        FIXED_TRIG_SCALE;
    forwardMotion = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) /
        FIXED_TRIG_SCALE;
    drive->accelPos = WrapSigned32(
        (int64_t)bodySin * forwardMotion) / FIXED_TRIG_SCALE;
    drive->brakePos = WrapSigned32(
        (int64_t)bodyCos * forwardMotion) / FIXED_TRIG_SCALE;
    return sidewaysMotion;
}

static void UpdateDrivingRedlineEffect(const PlayerCarRuntime *car,
                                       const GameCarSpec *spec) {
    const GameCarDrive *drive = &car->drive;
    s32 voiceLevel;

    if (drive->engineRpm <= WrapSigned32(
            (int64_t)spec->redline + REDLINE_EFFECT_RPM_MARGIN) ||
        g_SteerHoldFrames < REDLINE_EFFECT_HOLD_FRAMES ||
        drive->gear != spec->topGear ||
        car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    voiceLevel = g_SteerHoldFrames + REDLINE_EFFECT_VOLUME_BASE;
    if (voiceLevel > REDLINE_EFFECT_MAX_LEVEL) {
        voiceLevel = REDLINE_EFFECT_MAX_LEVEL;
    }
    SetIndexedEffectVoice(
        REDLINE_EFFECT_VOICE, REDLINE_EFFECT_PHASE, voiceLevel);
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
        drive->launchEnergy = WrapSigned32(
            (int64_t)car->speed * drive->coastFrames);
        drive->coastFrames = 0;
        if (launchThreshold->initial < car->speed &&
            drive->launchEnergy > drive->launchEnergyThreshold) {
            s32 gripDelta = WrapSigned32(
                (int64_t)drive->steeringGripResponse - STEERING_GRIP_BASE);
            s32 spinScale = WrapSigned32(
                (int64_t)STEERING_GRIP_BASE -
                WrapSigned32(
                    (int64_t)gripDelta * STEERING_GRIP_SPIN_SCALE));
            s32 reversedSideways;
            s32 spinRate;

            if (spinScale < STEERING_GRIP_BASE) {
                spinScale = STEERING_GRIP_BASE;
            }
            reversedSideways = WrapSigned32(-(int64_t)sidewaysMotion);
            spinRate = WrapSigned32(
                (int64_t)reversedSideways * spinScale) /
                STEERING_GRIP_BASE;
            BeginDrivingTakeoff(
                car, WrapSigned32(
                    (int64_t)spinRate * TAKEOFF_SPIN_MULTIPLIER));
        }
        return;
    }

    if (drive->acceleratorInput.value >= PEDAL_INPUT_RELEASED) {
        drive->coastFrames = 0;
        drive->launchEnergy = 0;
        return;
    }

    if (drive->brakeLatch == 1) {
        s32 magnitude = sidewaysMotion < 0
            ? WrapSigned32(-(int64_t)sidewaysMotion)
            : sidewaysMotion;
        s32 sidewaysSpeed = WrapSigned32(
            (int64_t)magnitude * car->speed) /
            BRAKING_SIDEWAYS_SPEED_DIVISOR;

        drive->launchEnergy = sidewaysSpeed;
        if (launchThreshold->sustain < car->speed &&
            drive->launchEnergyThreshold < sidewaysSpeed) {
            BeginDrivingTakeoff(car, -sidewaysMotion);
        }
        return;
    }

    drive->coastFrames = WrapSigned32(
        (int64_t)drive->coastFrames + 1);
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
