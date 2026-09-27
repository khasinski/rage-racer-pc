#include "game/car_drive.h"
#include "game/car_shift.h"
#include "game/integer.h"

enum {
    PEDAL_LATCH_ARM_THRESHOLD = 0x85,
    PEDAL_LATCH_RELEASE_THRESHOLD = 0x7C,
    ENGINE_RPM_LIMIT = 0x3A98,
    STANDING_START_ACCELERATOR_THRESHOLD = 0x40,
    STANDING_START_BRAKE_THRESHOLD = 0x80,
    GRIP_INPUT_PERCENT = 100,
    PEDAL_FIXED_SCALE = 256,
    PEDAL_FIXED_SHIFT = 8,
    PEDAL_FIXED_ROUNDING_BIAS = 0xFF,
    BASE_GRIP_BUDGET = 0x17C,
    TAKEOFF_BRAKE_SCALE = 20,
    TAKEOFF_RESISTANCE_SCALE = 2,
    TAKEOFF_SPEED_SCALE = 0x26FC,
    TAKEOFF_SPEED_DIVISOR = 10000,
    TAKEOFF_TORQUE_ROUNDING_BIAS = 0x1FFFFF,
    TAKEOFF_TORQUE_SHIFT = 21,
    AIRBORNE_SPEED_RETENTION = 0x3E7,
    PER_THOUSAND_SCALE = 1000,
    DRIVING_TORQUE_ROUNDING_BIAS = 0x1FFFF,
    DRIVING_TORQUE_SHIFT = 17,
    GRIP_LOSS_ACCELERATION_DIVISOR = 2,
    DRIVING_SPEED_RETENTION_PERCENT = 94,
    PERCENT_SCALE = 100,
};

typedef struct DrivetrainGearData {
    const s32 *torqueCurve;
    s32 ratio;
} DrivetrainGearData;

/*
 * A pedal's three-state latch. Pressing past 0x85 arms it, the next frame
 * confirms it, and it only clears once the pedal is back under 0x7C. The gap
 * between the two thresholds is what stops a pedal resting on the edge from
 * rattling the latch every frame.
 */
static void LatchPedal(s16 *latch, s32 input) {
    if (*latch == 0) {
        if (input >= PEDAL_LATCH_ARM_THRESHOLD) {
            *latch = 1;
        }
    } else if (*latch == 1) {
        *latch = 2;
    } else if (input < PEDAL_LATCH_RELEASE_THRESHOLD) {
        *latch = 0;
    }
}

static DrivetrainGearData SelectDrivetrainGearData(GameCarDrive *drive,
                                                    const GameCarSpec *spec,
                                                    const CarPerformance *performance,
                                                    int started) {
    DrivetrainGearData data = {
        .torqueCurve = performance->curves[drive->gear].values,
        .ratio = GetCarGearLoad(spec, drive->gear),
    };

    if (!started) {
        drive->gearDisp = drive->gear;
        data.ratio = GetCarGearLoad(spec, CAR_FIRST_FORWARD_GEAR);
        data.torqueCurve = performance->curves[0].values;
    } else if (drive->motionState == CAR_MOTION_STANDING_START &&
               (drive->acceleratorInput.value <
                    STANDING_START_ACCELERATOR_THRESHOLD ||
                drive->brakeInput >= STANDING_START_BRAKE_THRESHOLD)) {
        data.torqueCurve = performance->curves[0].values;
    }
    return data;
}

static s32 UpdatePedalLatchesAndGrip(GameCarDrive *drive) {
    s32 acceleratorGripNumerator;
    s32 acceleratorGripCost;
    s32 gripBudget;

    LatchPedal(&drive->acceleratorLatch, drive->acceleratorInput.value);
    LatchPedal(&drive->brakeLatch, drive->brakeInput);
    acceleratorGripNumerator = WrapSigned32(
        (int64_t)drive->acceleratorInput.value * GRIP_INPUT_PERCENT);
    acceleratorGripCost = acceleratorGripNumerator >> PEDAL_FIXED_SHIFT;
    if (acceleratorGripNumerator < 0) {
        acceleratorGripCost =
            (acceleratorGripNumerator + PEDAL_FIXED_ROUNDING_BIAS) >>
            PEDAL_FIXED_SHIFT;
    }
    gripBudget = WrapSigned32(
        (int64_t)BASE_GRIP_BUDGET - acceleratorGripCost);
    return WrapSigned32(
        (int64_t)gripBudget +
        WrapSigned32(
            (int64_t)drive->brakeInput * GRIP_INPUT_PERCENT) /
            PEDAL_FIXED_SCALE);
}

static void UpdateEngineRpm(GameCarDrive *drive,
                            const CarDrivetrainLoads *loads) {
    if (drive->jumpTimer <= 0 && drive->clutch <= 0) {
        drive->engineRpm = WrapSigned32(
            (int64_t)drive->engineRpm + loads->throttleAcceleration);
        drive->engineRpm = WrapSigned32(
            (int64_t)drive->engineRpm - loads->longitudinalResistance);
        drive->engineRpm = WrapSigned32(
            (int64_t)drive->engineRpm - loads->motionResistance);
    }
    if (drive->engineRpm < 0) {
        drive->engineRpm = 0;
    } else if (drive->engineRpm > ENGINE_RPM_LIMIT) {
        drive->engineRpm = ENGINE_RPM_LIMIT;
    }
}

static void AlignStoppedCarHeading(PlayerCarRuntime *car) {
    if (car->speed < CAR_STOPPED_SPEED_THRESHOLD) {
        car->headingAngle = car->bodyYaw;
    }
}

static void UpdateTakeoffSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                               s32 motionResistance) {
    s32 brakeDrag = WrapSigned32(
        (int64_t)drive->brakeInput * TAKEOFF_BRAKE_SCALE);
    s32 resistanceScale = WrapSigned32(
        (int64_t)motionResistance * TAKEOFF_RESISTANCE_SCALE);
    s32 coefficient = WrapSigned32(
        (int64_t)(TAKEOFF_SPEED_SCALE - 1) - resistanceScale);
    s32 speedScale;
    s32 torque = drive->drivetrainTorque;

    if (brakeDrag < 0) {
        brakeDrag += PEDAL_FIXED_ROUNDING_BIAS;
    }
    speedScale = WrapSigned32(
        (int64_t)coefficient - (brakeDrag >> PEDAL_FIXED_SHIFT));
    car->speed = WrapSigned32(
        (int64_t)speedScale * car->speed) / TAKEOFF_SPEED_DIVISOR;
    if (torque < 0) {
        torque += TAKEOFF_TORQUE_ROUNDING_BIAS;
    }
    car->acceleration = torque >> TAKEOFF_TORQUE_SHIFT;
}

static void UpdateDrivenSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                              const GameCarSpec *spec, s32 gearTorque) {
    s32 speedScale;

    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        car->acceleration = 0;
        speedScale = AIRBORNE_SPEED_RETENTION;
        car->speed = WrapSigned32(
            (int64_t)car->speed * speedScale) / PER_THOUSAND_SCALE;
        return;
    }

    if (drive->clutch > 0 || drive->jumpTimer > 0) {
        car->acceleration = drive->engineLoad;
    } else {
        s32 shiftedTorque = gearTorque;

        if (shiftedTorque < 0) {
            shiftedTorque += DRIVING_TORQUE_ROUNDING_BIAS;
        }
        shiftedTorque >>= DRIVING_TORQUE_SHIFT;
        car->acceleration = drive->manual != 0
            ? shiftedTorque
            : WrapSigned32(
                  (int64_t)spec->automaticAccelerationScale * shiftedTorque) /
                  PER_THOUSAND_SCALE;
    }
    if (drive->gripLossTimer > 0) {
        car->acceleration /= GRIP_LOSS_ACCELERATION_DIVISOR;
    }
    car->speed = WrapSigned32(
        (int64_t)car->speed * DRIVING_SPEED_RETENTION_PERCENT) /
        PERCENT_SCALE;
}

void StepCarDrivetrain(PlayerCarRuntime *car, const GameCarSpec *spec,
                       const CarPerformance *performance,
                       const DriveContext *context) {
    GameCarDrive *drive = &car->drive;
    s16 gear = ClampCarGear(drive->gear, CAR_FORWARD_GEAR_COUNT);
    DrivetrainGearData gearData;
    s32 gripBudget;
    s32 initialAcceleration;
    s32 bandScale;
    s32 netTorque;
    s32 gearTorque;
    CarDrivetrainLoads loads;
    drive->gear = gear;
    gearData = SelectDrivetrainGearData(drive, spec, performance, context->started);
    gripBudget = UpdatePedalLatchesAndGrip(drive);
    UpdateCarSteeringGrip(car, spec, context, gripBudget);

    initialAcceleration =
        CalculateCarInitialAcceleration(drive, gearData.ratio);
    /* If RPM falls between configured bands, retail keeps the raw wheel/load
     * difference rather than replacing it with an interpolated curve value. */
    netTorque = WrapSigned32(
        (int64_t)WrapSigned32(
            (int64_t)gearData.ratio * drive->engineRpm) -
        drive->drivetrainTorque);
    ReadCarEngineTorque(drive, spec, performance, gearData.torqueCurve,
                        &netTorque, &bandScale);
    UpdateCarGearShiftState(car, spec, drive->roadGrade, &initialAcceleration);
    loads = CalculateCarDrivetrainLoads(
        car, spec, context, netTorque, bandScale, initialAcceleration);
    UpdateEngineRpm(drive, &loads);

    gearTorque = WrapSigned32(
        (int64_t)gearData.ratio * drive->engineRpm);
    drive->drivetrainTorque = gearTorque;
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        UpdateTakeoffSpeed(car, drive, loads.motionResistance);
    } else {
        UpdateDrivenSpeed(car, drive, spec, gearTorque);
    }

    if (!context->started) {
        car->speed = 0;
    }
    AlignStoppedCarHeading(car);
    /* Remember the applied gear in physics, even when no presentation runs. */
    drive->gearDisp = drive->gear;
}
