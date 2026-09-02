#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
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
    TRIG_FIXED_ONE = 4096,
    TRAVEL_VELOCITY_DIVISOR = 256,
    BODY_VELOCITY_DIVISOR = 16384,
    PEDAL_INPUT_FULL = 256,
};

static void AlignStandingStartVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    car->bodyYaw += GetAngleDelta(car->bodyYaw, drive->targetHeading) /
                    STANDING_START_YAW_RESPONSE;
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    drive->accelPos =
        rsin(car->headingAngle) * car->speed / TRAVEL_VELOCITY_DIVISOR;
    drive->brakePos =
        rcos(car->headingAngle) * car->speed / TRAVEL_VELOCITY_DIVISOR;
    alongBody = (bodySin * drive->accelPos + bodyCos * drive->brakePos) /
                TRIG_FIXED_ONE;
    drive->accelPos = bodySin * alongBody / BODY_VELOCITY_DIVISOR;
    drive->brakePos = bodyCos * alongBody / BODY_VELOCITY_DIVISOR;
}

static int UpdateStandingStartWheelspin(GameCarDrive *drive) {
    s32 throttle;
    s32 rpm;
    s32 grip;

    if (g_StandingStartSpin < STANDING_START_SPIN_THRESHOLD) return 0;

    throttle = drive->acceleratorInput.value;
    rpm = drive->engineRpm;
    grip = throttle + STANDING_START_BASE_GRIP;

    g_StandingStartSpin -= drive->brakeInput * 2;
    if (rpm < STANDING_START_LOW_RPM) {
        grip += STANDING_START_LOW_RPM_GRIP_BONUS;
    } else if (rpm > STANDING_START_LOW_RPM &&
               throttle < STANDING_START_LOW_THROTTLE) {
        grip += STANDING_START_LOW_THROTTLE;
    }

    drive->standingStartBounceY =
        (Random15() & 3) * grip / PEDAL_INPUT_FULL;
    drive->standingStartBounceX =
        (Random15() & 7) * grip / PEDAL_INPUT_FULL;
    g_StandingStartSpin -= grip;
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
         (g_StandingStartSpin & STANDING_START_EFFECT_SPIN_MASK) * 2) *
            drive->acceleratorInput.value / PEDAL_INPUT_FULL);
    car->speed /= STANDING_START_SPEED_DAMPING;

    if (UpdateStandingStartWheelspin(drive)) return;
    FinishStandingStart(drive);
}
