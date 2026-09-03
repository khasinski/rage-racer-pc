#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "psyq/gte.h"

enum {
    LAUNCH_STEERING_CENTRE_LIMIT = 127,
    LAUNCH_STEERING_CENTRE_WIDTH =
        LAUNCH_STEERING_CENTRE_LIMIT * 2 + 1,
    LAUNCH_SPIN_LIMIT = 0x3600,
    LAUNCH_RECOVERY_SPIN_THRESHOLD = 0x1000,
    TYRE_SKID_PHASE_LIMIT = 513,
    TYRE_SKID_PHASE_SCALE = 3,
    TYRE_SKID_PHASE_BASE = 0x1800,
    TYRE_SKID_PHASE_MAXIMUM = 0x1E00,
    TYRE_SKID_VOLUME_DIVISOR = 8,
    TYRE_SKID_VOLUME_BASE = 0x40,
    TYRE_SKID_VOLUME_MAXIMUM = 0x7F,
    INITIAL_SKID_LIMIT = 0x80,
    LOW_SPIN_THRESHOLD = 0x800,
    INITIAL_SPIN_ENERGY_SCALE = 4000,
    INITIAL_SPIN_ENERGY_DIVISOR = 256,
    LOW_SPEED_THRESHOLD = 0x190,
    LOW_SPEED_ENERGY_SCALE = 100,
    BODY_LIFT_STEP = 10,
    BODY_LIFT_LIMIT = 100,
    TARGET_YAW_RETENTION_PERCENT = 98,
    PERCENT_SCALE = 100,
    STEERING_LOAD_CENTER = 0x800,
    SPIN_ACCELERATION_SCALE = 16,
    HEADING_ALIGNMENT_LIMIT = 0x200,
    POWERED_SPIN_DAMPING_NUMERATOR = 31,
    POWERED_SPIN_DAMPING_DENOMINATOR = 32,
    SPIN_CORRECTION_DIVISOR = 2,
    BODY_YAW_SPIN_DIVISOR = 256,
    BASE_LAUNCH_ENERGY_COST = 64,
    SKID_ENERGY_DIVISOR = 65536,
    SPIN_ENERGY_DIVISOR = 64,
    SPEED_ENERGY_DIVISOR = 8,
    PEDAL_FULLY_PRESSED = 0x100,
    PEDAL_ENERGY_SCALE = 4,
    PEDAL_SPEED_SCALE = 10,
    PEDAL_SPEED_DIVISOR = 256,
    DEPLETED_SPIN_DAMPING_NUMERATOR = 15,
    DEPLETED_SPIN_DAMPING_DENOMINATOR = 16,
    QUARTER_TURN_BOUNDARY = 0x401,
    HALF_TURN = 0x800,
    LAUNCH_SPEED_NORMALIZER = 0x100000,
    QUIET_SHIFT_RPM_DELTA = 99,
    LARGE_SKID_OFFSET = 0x400,
    LARGE_SKID_FORCE_SCALE = 4,
    LARGE_SKID_FORCE_DIVISOR = LAUNCH_SPIN_LIMIT * 1024,
    SMALL_SKID_FORCE_BASE = 0x200,
    SMALL_SKID_FORCE_DIVISOR = 512,
    VELOCITY_COMPONENT_DIVISOR = 256,
    SEVERE_SKID_THRESHOLD = 0x600,
    SEVERE_SKID_SPEED_RETENTION = 990,
    PER_THOUSAND_SCALE = 1000,
};

static s32 AbsoluteSpinRate(s32 spinRate) {
    return spinRate < 0 ? WrapSigned32(-(int64_t)spinRate) : spinRate;
}

/* This form preserves the original wrapped range test even for corrupted
 * steering values near INT32_MAX. For normal input it means -127..127. */
static s32 IsLaunchSteeringCentred(s32 steerPos) {
    return (u32)WrapSigned32(
               (int64_t)steerPos + LAUNCH_STEERING_CENTRE_LIMIT) <
           LAUNCH_STEERING_CENTRE_WIDTH;
}

static void UpdateLaunchTyreVoice(const PlayerCarRuntime *car, s32 skid) {
    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    if (skid < TYRE_SKID_PHASE_LIMIT) {
        SetIndexedEffectVoice(
            0, skid * TYRE_SKID_PHASE_SCALE + TYRE_SKID_PHASE_BASE,
            skid / TYRE_SKID_VOLUME_DIVISOR + TYRE_SKID_VOLUME_BASE);
    } else {
        SetIndexedEffectVoice(0, TYRE_SKID_PHASE_MAXIMUM,
                              TYRE_SKID_VOLUME_MAXIMUM);
    }
}

static void ConsumeInitialLaunchEnergy(PlayerCarRuntime *car, s32 skid,
                                       s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;

    if (skid < INITIAL_SKID_LIMIT && spinMagnitude < LOW_SPIN_THRESHOLD) {
        s32 deficit = WrapSigned32(
            (int64_t)LOW_SPIN_THRESHOLD - spinMagnitude);
        s32 cost = WrapSigned32(
            (int64_t)deficit * INITIAL_SPIN_ENERGY_SCALE) /
            INITIAL_SPIN_ENERGY_DIVISOR;

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy - cost);
    }
    if (car->speed < LOW_SPEED_THRESHOLD) {
        s32 deficit = WrapSigned32(
            (int64_t)LOW_SPEED_THRESHOLD - car->speed);
        s32 cost = WrapSigned32(
            (int64_t)deficit * LOW_SPEED_ENERGY_SCALE);

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy - cost);
    }
}

static void UpdatePoweredLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 towardsTarget;
    s32 skid;

    drive->bodyLiftOffset = WrapSigned16(
        (s32)drive->bodyLiftOffset + BODY_LIFT_STEP);
    if (drive->bodyLiftOffset >= BODY_LIFT_LIMIT) {
        drive->bodyLiftOffset = BODY_LIFT_LIMIT;
    }

    towardsTarget =
        GetAngleDelta(car->bodyYaw, drive->targetHeading) *
        TARGET_YAW_RETENTION_PERCENT / PERCENT_SCALE;
    towardsTarget = WrapSigned32(
        (int64_t)towardsTarget * WrapSigned32(
            (int64_t)drive->steeringLoadAngle + STEERING_LOAD_CENTER)) /
        STEERING_LOAD_CENTER;
    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate +
        WrapSigned32((int64_t)towardsTarget * SPIN_ACCELERATION_SCALE));

    if (IsLaunchSteeringCentred(drive->steerPos)) {
        if (GetAngleDistance(car->bodyYaw, car->headingAngle) <
            HEADING_ALIGNMENT_LIMIT) {
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate *
                POWERED_SPIN_DAMPING_NUMERATOR) /
                POWERED_SPIN_DAMPING_DENOMINATOR;
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate +
                GetAngleDelta(car->bodyYaw, car->headingAngle));
        } else if (spinMagnitude < LOW_SPIN_THRESHOLD) {
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate +
                towardsTarget / SPIN_CORRECTION_DIVISOR);
        }
    }

    if (drive->spinRate > LAUNCH_SPIN_LIMIT) {
        drive->spinRate = LAUNCH_SPIN_LIMIT;
    }
    if (drive->spinRate < -LAUNCH_SPIN_LIMIT) {
        drive->spinRate = -LAUNCH_SPIN_LIMIT;
    }
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw + drive->spinRate / BODY_YAW_SPIN_DIVISOR);

    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - BASE_LAUNCH_ENERGY_COST);
    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        skid * skid / SKID_ENERGY_DIVISOR);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        WrapSigned32((int64_t)LAUNCH_SPIN_LIMIT - spinMagnitude) /
            SPIN_ENERGY_DIVISOR);
    if (car->speed < drive->speedScale / 2) {
        s32 speedDeficit = WrapSigned32(
            (int64_t)(drive->speedScale / 2) - car->speed);

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy -
            speedDeficit / SPEED_ENERGY_DIVISOR);
    }
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        drive->brakeInput * PEDAL_ENERGY_SCALE);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        (PEDAL_FULLY_PRESSED - drive->acceleratorInput.value) *
            PEDAL_ENERGY_SCALE);

    car->speed = WrapSigned32(
        (int64_t)car->speed -
        drive->brakeInput * PEDAL_SPEED_SCALE / PEDAL_SPEED_DIVISOR);
    car->speed = WrapSigned32(
        (int64_t)car->speed -
        (PEDAL_FULLY_PRESSED - drive->acceleratorInput.value) *
            PEDAL_SPEED_SCALE / PEDAL_SPEED_DIVISOR);
}

static void UpdateDepletedLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 offAxis;

    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate * DEPLETED_SPIN_DAMPING_NUMERATOR) /
        DEPLETED_SPIN_DAMPING_DENOMINATOR;
    if (spinMagnitude >= LAUNCH_RECOVERY_SPIN_THRESHOLD) {
        return;
    }

    drive->yawOffset = GetAngleDelta(car->headingAngle, car->bodyYaw);
    drive->launchHeading = car->headingAngle;
    car->headingAngle = car->bodyYaw;

    offAxis = drive->yawOffset < 0
        ? WrapSigned32(-(int64_t)drive->yawOffset)
        : drive->yawOffset;
    if (offAxis < QUARTER_TURN_BOUNDARY) {
        offAxis = WrapSigned32((int64_t)offAxis * offAxis);
    } else {
        s32 distance = WrapSigned32((int64_t)HALF_TURN - offAxis);

        offAxis = WrapSigned32((int64_t)distance * distance);
    }
    drive->launchSpeed = WrapSigned32(
        (int64_t)offAxis * car->speed) / LAUNCH_SPEED_NORMALIZER;
    drive->spinRate = 0;

    PrepareAirborneDrivetrain(car);
    g_ShiftSoundLevel =
        drive->shiftRpmDelta >= -QUIET_SHIFT_RPM_DELTA &&
        drive->shiftRpmDelta <= QUIET_SHIFT_RPM_DELTA;
}

static void FinishLaunchFrame(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 skid;
    s32 launchedHeading;

    drive->targetHeading = car->bodyYaw;
    SteerCarToTrackLine(car);

    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    if (skid >= QUARTER_TURN_BOUNDARY) {
        s32 push = car->acceleration;
        s32 scaled = WrapSigned32(
            (int64_t)LAUNCH_SPIN_LIMIT - spinMagnitude);

        scaled = WrapSigned32(
            (int64_t)scaled * LARGE_SKID_FORCE_SCALE);
        scaled = WrapSigned32((int64_t)scaled * push);
        scaled = WrapSigned32(
            (int64_t)scaled * (skid - LARGE_SKID_OFFSET));
        car->acceleration = WrapSigned32(
            (int64_t)(push / SPIN_CORRECTION_DIVISOR) +
            scaled / LARGE_SKID_FORCE_DIVISOR);
    } else {
        car->acceleration = WrapSigned32(
            (int64_t)(SMALL_SKID_FORCE_BASE - skid) * car->acceleration) /
            SMALL_SKID_FORCE_DIVISOR;
    }

    launchedHeading = car->headingAngle;
    UpdateCarTravelVelocity(AsRivalCar(car));
    car->headingAngle = launchedHeading;
    drive->accelPos = WrapSigned32(
        (int64_t)rsin(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)rcos(car->headingAngle) * car->speed) /
        VELOCITY_COMPONENT_DIVISOR;
}

/* Update the takeoff state until its energy is spent, then hand the car to
 * UpdateCarAirborne. */
void UpdateCarLaunch(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 startYaw = car->bodyYaw;
    s32 startHeading = car->headingAngle;
    s32 spinMagnitude = AbsoluteSpinRate(drive->spinRate);
    s32 skid;

    skid = GetAngleDistance(startYaw, startHeading);
    if (skid >= SEVERE_SKID_THRESHOLD) {
        car->speed = WrapSigned32(
            (int64_t)car->speed * SEVERE_SKID_SPEED_RETENTION) /
            PER_THOUSAND_SCALE;
    }
    UpdateLaunchTyreVoice(car, skid);
    ConsumeInitialLaunchEnergy(car, skid, spinMagnitude);

    if (drive->launchEnergy > 0) {
        UpdatePoweredLaunch(car, spinMagnitude);
    } else {
        UpdateDepletedLaunch(car, spinMagnitude);
    }

    FinishLaunchFrame(car, spinMagnitude);
}
