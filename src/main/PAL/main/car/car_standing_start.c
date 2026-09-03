#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/random.h"
#include "psyq/gte.h"

enum {
    STANDING_START_YAW_RESPONSE = 5,
    STANDING_START_SPIN_THRESHOLD = 11,
    STANDING_START_LOW_RPM = 2000,
    STANDING_START_LOW_THROTTLE = 127,
    STANDING_START_BASE_GRIP = 32,
    STANDING_START_LOW_RPM_GRIP_BONUS = 1000,
    STANDING_START_SPEED_DAMPING = 10,
    STANDING_START_EFFECT_PHASE = 0x1A80,
    STANDING_START_EFFECT_BASE_VOLUME = 0x60,
    STANDING_START_EFFECT_SPIN_MASK = 0x1F,
    STANDING_START_EFFECT_SPIN_SCALE = 2,
    TRIG_FIXED_ONE = 4096,
    TRAVEL_VELOCITY_DIVISOR = 256,
    BODY_VELOCITY_DIVISOR = 16384,
    PEDAL_INPUT_FULL = 256,
    BRAKE_SPIN_REDUCTION_SCALE = 2,
    VERTICAL_BOUNCE_RANDOM_MASK = 3,
    LATERAL_BOUNCE_RANDOM_MASK = 7,
};

static void AlignStandingStartVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, drive->targetHeading) /
            STANDING_START_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    drive->accelPos = WrapSigned32(
        (int64_t)rsin(car->headingAngle) * car->speed) /
        TRAVEL_VELOCITY_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)rcos(car->headingAngle) * car->speed) /
        TRAVEL_VELOCITY_DIVISOR;
    alongBody = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) /
        TRIG_FIXED_ONE;
    drive->accelPos = WrapSigned32(
        (int64_t)bodySin * alongBody) / BODY_VELOCITY_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)bodyCos * alongBody) / BODY_VELOCITY_DIVISOR;
}

static int UpdateStandingStartWheelspin(GameCarDrive *drive) {
    s32 throttle;
    s32 rpm;
    s32 grip;

    if (g_StandingStartSpin < STANDING_START_SPIN_THRESHOLD) {
        return 0;
    }

    throttle = drive->acceleratorInput.value;
    rpm = drive->engineRpm;
    grip = throttle + STANDING_START_BASE_GRIP;

    g_StandingStartSpin = WrapSigned32(
        (int64_t)g_StandingStartSpin -
        drive->brakeInput * BRAKE_SPIN_REDUCTION_SCALE);
    if (rpm < STANDING_START_LOW_RPM) {
        grip += STANDING_START_LOW_RPM_GRIP_BONUS;
    } else if (rpm > STANDING_START_LOW_RPM &&
               throttle < STANDING_START_LOW_THROTTLE) {
        grip += STANDING_START_LOW_THROTTLE;
    }

    drive->standingStartBounceY =
        (Random15() & VERTICAL_BOUNCE_RANDOM_MASK) * grip /
        PEDAL_INPUT_FULL;
    drive->standingStartBounceX =
        (Random15() & LATERAL_BOUNCE_RANDOM_MASK) * grip /
        PEDAL_INPUT_FULL;
    g_StandingStartSpin = WrapSigned32(
        (int64_t)g_StandingStartSpin - grip);
    return g_StandingStartSpin > 0;
}

static void FinishStandingStart(GameCarDrive *drive) {
    drive->standingStartBounceY = 0;
    drive->standingStartBounceX = 0;
    drive->motionState = CAR_MOTION_DRIVING;
    SetIndexedEffectVoice(-1, 0, 0);
}

void UpdateCarStandingStart(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    AlignStandingStartVelocity(car);

    SetIndexedEffectVoice(
        0, STANDING_START_EFFECT_PHASE,
        (STANDING_START_EFFECT_BASE_VOLUME -
         (g_StandingStartSpin & STANDING_START_EFFECT_SPIN_MASK) *
             STANDING_START_EFFECT_SPIN_SCALE) *
            drive->acceleratorInput.value / PEDAL_INPUT_FULL);
    car->speed /= STANDING_START_SPEED_DAMPING;

    if (UpdateStandingStartWheelspin(drive)) {
        return;
    }
    FinishStandingStart(drive);
}
