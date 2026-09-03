#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "psyq/gte.h"

enum {
    AIRBORNE_YAW_RESPONSE = 5,
    AIRBORNE_SKID_PHASE_LIMIT = 513,
    AIRBORNE_LARGE_YAW = 1537,
    AIRBORNE_INPUT_RELEASED = 128,
    AIRBORNE_DECAY_NUMERATOR = 31,
    AIRBORNE_DECAY_DENOMINATOR = 32,
    AIRBORNE_SKID_PHASE_BASE = 0x1800,
    AIRBORNE_SKID_PHASE_MAXIMUM = 0x1E00,
    AIRBORNE_SKID_PHASE_PER_YAW = 3,
    AIRBORNE_TIMER_VOLUME_SCALE = 2,
    AIRBORNE_TIMER_VOLUME_BASE = 80,
    AIRBORNE_SHIFT_VOLUME_BASE = 25,
    AIRBORNE_VELOCITY_SCALE = 256,
    FIXED_TRIG_SCALE = 4096,
    LARGE_YAW_SPEED_NUMERATOR = 4,
    LARGE_YAW_SPEED_DENOMINATOR = 5,
    BODY_LIFT_DECAY_NUMERATOR = 2,
    BODY_LIFT_DECAY_DENOMINATOR = 3,
};

static s32 AbsoluteYawOffset(s32 yawOffset) {
    return yawOffset < 0
        ? WrapSigned32(-(int64_t)yawOffset)
        : yawOffset;
}

static void UpdateAirborneTyreVoice(const GameCarDrive *drive) {
    if (g_ShiftSoundLevel == 0) {
        s32 offAxis = AbsoluteYawOffset(drive->yawOffset);
        s32 phase = offAxis < AIRBORNE_SKID_PHASE_LIMIT
                        ? WrapSigned32(
                              (int64_t)offAxis *
                                  AIRBORNE_SKID_PHASE_PER_YAW +
                              AIRBORNE_SKID_PHASE_BASE)
                        : AIRBORNE_SKID_PHASE_MAXIMUM;

        SetIndexedEffectVoice(
            0, phase,
            drive->jumpTimer * AIRBORNE_TIMER_VOLUME_SCALE +
                AIRBORNE_TIMER_VOLUME_BASE);
    } else {
        SetIndexedEffectVoice(
            0, AIRBORNE_SKID_PHASE_BASE,
            WrapSigned32((int64_t)g_ShiftSoundLevel +
                         AIRBORNE_SHIFT_VOLUME_BASE));
    }
}

static s32 AirborneVelocityComponent(s32 trig, s32 speed) {
    return WrapSigned32((int64_t)trig * speed) / AIRBORNE_VELOCITY_SCALE;
}

static void UpdateAirborneVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 motionHeading;
    s32 alongBody;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, drive->targetHeading) /
            AIRBORNE_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    motionHeading = WrapSigned32(
        (int64_t)car->headingAngle + drive->yawOffset);
    drive->accelPos = AirborneVelocityComponent(
        rsin(motionHeading), car->speed);
    drive->brakePos = AirborneVelocityComponent(
        rcos(motionHeading), car->speed);
    alongBody = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) /
        FIXED_TRIG_SCALE;

    drive->accelPos = WrapSigned32(
        (int64_t)AirborneVelocityComponent(
            rsin(drive->launchHeading), drive->launchSpeed) +
        WrapSigned32((int64_t)bodySin * alongBody) / FIXED_TRIG_SCALE);
    drive->brakePos = WrapSigned32(
        (int64_t)AirborneVelocityComponent(
            rcos(drive->launchHeading), drive->launchSpeed) +
        WrapSigned32((int64_t)bodyCos * alongBody) / FIXED_TRIG_SCALE);
}

static void UpdateAirborneCoastFrames(GameCarDrive *drive) {
    if (drive->acceleratorLatch != 1 && drive->brakeLatch != 1 &&
        drive->acceleratorInput.value < AIRBORNE_INPUT_RELEASED) {
        drive->coastFrames = WrapSigned32(
            (int64_t)drive->coastFrames + 1);
    } else {
        drive->coastFrames = 0;
    }
}

static void DecayAirborneMotion(GameCarDrive *drive) {
    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->launchSpeed = WrapSigned32(
        (int64_t)drive->launchSpeed * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->yawOffset = WrapSigned32(
        (int64_t)drive->yawOffset * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->bodyLiftOffset =
        drive->bodyLiftOffset * BODY_LIFT_DECAY_NUMERATOR /
        BODY_LIFT_DECAY_DENOMINATOR;
}

static void FinishAirborneMotion(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    SetIndexedEffectVoice(-1, 0, 0);
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw - drive->spinRate);
    g_ShiftSoundLevel = 0;
    drive->shiftRpmDelta = 0;
    drive->yawOffset = 0;
    drive->launchSpeed = 0;
    drive->motionState = CAR_MOTION_DRIVING;
    drive->bodyLiftOffset = 0;
}

void UpdateCarAirborne(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    UpdateAirborneTyreVoice(drive);
    UpdateAirborneVelocity(car);
    UpdateAirborneCoastFrames(drive);
    DecayAirborneMotion(drive);

    if (AbsoluteYawOffset(drive->yawOffset) >= AIRBORNE_LARGE_YAW) {
        car->speed = WrapSigned32(
            (int64_t)car->speed * LARGE_YAW_SPEED_NUMERATOR) /
            LARGE_YAW_SPEED_DENOMINATOR;
    }
    if (drive->jumpTimer <= 0) {
        FinishAirborneMotion(car);
    }
}
