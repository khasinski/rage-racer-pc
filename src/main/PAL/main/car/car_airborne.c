#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/integer.h"
#include "psyq/gte.h"

enum {
    AIRBORNE_YAW_RESPONSE = 5,
    AIRBORNE_SKID_PHASE_LIMIT = 513,
    AIRBORNE_LARGE_YAW = 1537,
    AIRBORNE_INPUT_RELEASED = 128,
    AIRBORNE_DECAY_NUMERATOR = 31,
    AIRBORNE_DECAY_DENOMINATOR = 32,
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
                        ? WrapSigned32((int64_t)offAxis * 3 + 0x1800)
                        : 0x1E00;

        SetIndexedEffectVoice(0, phase, drive->jumpTimer * 2 + 80);
    } else {
        SetIndexedEffectVoice(0, 0x1800, g_ShiftSoundLevel + 25);
    }
}

static void UpdateAirborneVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, drive->targetHeading) /
            AIRBORNE_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    {
        s32 motionHeading = WrapSigned32(
            (int64_t)car->headingAngle + drive->yawOffset);

        drive->accelPos = WrapSigned32(
            (int64_t)rsin(motionHeading) * car->speed) / 256;
        drive->brakePos = WrapSigned32(
            (int64_t)rcos(motionHeading) * car->speed) / 256;
    }
    alongBody = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) / 4096;

    drive->accelPos = WrapSigned32(
        (int64_t)(WrapSigned32(
            (int64_t)rsin(drive->launchHeading) * drive->launchSpeed) /
            256) +
        WrapSigned32((int64_t)bodySin * alongBody) / 4096);
    drive->brakePos = WrapSigned32(
        (int64_t)(WrapSigned32(
            (int64_t)rcos(drive->launchHeading) * drive->launchSpeed) /
            256) +
        WrapSigned32((int64_t)bodyCos * alongBody) / 4096);
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
    drive->bodyLiftOffset = drive->bodyLiftOffset * 2 / 3;
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
        car->speed = WrapSigned32((int64_t)car->speed * 4) / 5;
    }
    if (drive->jumpTimer <= 0) {
        FinishAirborneMotion(car);
    }
}
