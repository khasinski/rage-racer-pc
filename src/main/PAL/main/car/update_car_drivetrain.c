#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

enum {
    FIRST_FORWARD_GEAR = 1,
    PEDAL_LATCH_ARM_THRESHOLD = 0x85,
    PEDAL_LATCH_RELEASE_THRESHOLD = 0x7C,
    ENGINE_RPM_LIMIT = 0x3A98,
    STOPPED_SPEED_THRESHOLD = 8,
};

typedef struct DrivetrainGearData {
    const s32 *torqueCurve;
    s32 ratio;
} DrivetrainGearData;

static s16 ClampDrivetrainGear(s16 gear) {
    if (gear < FIRST_FORWARD_GEAR) {
        return FIRST_FORWARD_GEAR;
    }
    if (gear > CAR_FORWARD_GEAR_COUNT) {
        return CAR_FORWARD_GEAR_COUNT;
    }
    return gear;
}

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
                                                    const GameCarSpec *spec) {
    DrivetrainGearData data = {
        .torqueCurve = g_GearTorqueCurve[drive->gear].values,
        .ratio = GetCarGearLoad(spec, drive->gear),
    };

    if (g_RacePhase < 2) {
        drive->gearDisp = drive->gear;
        data.ratio = GetCarGearLoad(spec, FIRST_FORWARD_GEAR);
        data.torqueCurve = g_GearTorqueCurve[0].values;
    } else if (drive->motionState == CAR_MOTION_STANDING_START &&
               (drive->acceleratorInput.value < 0x40 ||
                drive->brakeInput >= 0x80)) {
        data.torqueCurve = g_GearTorqueCurve[0].values;
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
        (int64_t)drive->acceleratorInput.value * 100);
    acceleratorGripCost = acceleratorGripNumerator >> 8;
    if (acceleratorGripNumerator < 0) {
        acceleratorGripCost = (acceleratorGripNumerator + 0xFF) >> 8;
    }
    gripBudget = WrapSigned32((int64_t)0x17C - acceleratorGripCost);
    return WrapSigned32(
        (int64_t)gripBudget +
        WrapSigned32((int64_t)drive->brakeInput * 100) / 256);
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
    if (car->speed < STOPPED_SPEED_THRESHOLD) {
        car->headingAngle = car->bodyYaw;
    }
}

static void UpdateTakeoffSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                               s32 motionResistance) {
    s32 brakeDrag = WrapSigned32((int64_t)drive->brakeInput * 20);
    s32 resistanceScale = WrapSigned32((int64_t)motionResistance * 2);
    s32 coefficient = WrapSigned32((int64_t)(0x26FC - 1) - resistanceScale);
    s32 speedScale;
    s32 torque = drive->drivetrainTorque;

    if (brakeDrag < 0) {
        brakeDrag += 0xFF;
    }
    speedScale = WrapSigned32((int64_t)coefficient - (brakeDrag >> 8));
    car->speed = WrapSigned32((int64_t)speedScale * car->speed) / 10000;
    if (torque < 0) {
        torque += 0x1FFFFF;
    }
    car->acceleration = torque >> 21;
}

static void UpdateDrivenSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                              const GameCarSpec *spec, s32 gearTorque) {
    s32 speedScale;

    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        car->acceleration = 0;
        speedScale = 0x3E7;
        car->speed = WrapSigned32((int64_t)car->speed * speedScale) / 1000;
        return;
    }

    if (drive->clutch > 0 || drive->jumpTimer > 0) {
        car->acceleration = drive->engineLoad;
    } else {
        s32 shiftedTorque = gearTorque;

        if (shiftedTorque < 0) {
            shiftedTorque += 0x1FFFF;
        }
        shiftedTorque >>= 17;
        car->acceleration = drive->manual != 0
            ? shiftedTorque
            : WrapSigned32(
                  (int64_t)spec->automaticAccelerationScale * shiftedTorque) /
                  1000;
    }
    if (g_GripLossTimer > 0) {
        car->acceleration /= 2;
    }
    car->speed = WrapSigned32((int64_t)car->speed * 94) / 100;
}

static void DispatchCarMotion(PlayerCarRuntime *car) {
    switch (car->drive.motionState) {
    case CAR_MOTION_DRIVING:
        UpdateCarDriving(car);
        break;
    case CAR_MOTION_TAKEOFF:
        UpdateCarLaunch(car);
        break;
    case CAR_MOTION_AIRBORNE:
        UpdateCarAirborne(car);
        break;
    case CAR_MOTION_STANDING_START:
        UpdateCarStandingStart(car);
        break;
    }
}

void UpdateCarDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    s16 gear = ClampDrivetrainGear(drive->gear);
    DrivetrainGearData gearData;
    s32 gripBudget;
    s32 initialAcceleration;
    s32 bandScale;
    s32 netTorque;
    s32 gearTorque;
    CarDrivetrainLoads loads;

    drive->gear = gear;
    gearData = SelectDrivetrainGearData(drive, spec);
    gripBudget = UpdatePedalLatchesAndGrip(drive);
    UpdateCarSteeringGrip(car, spec, gripBudget);

    initialAcceleration =
        CalculateCarInitialAcceleration(drive, gearData.ratio);
    /* If RPM falls between configured bands, retail keeps the raw wheel/load
     * difference rather than replacing it with an interpolated curve value. */
    netTorque = WrapSigned32(
        (int64_t)WrapSigned32(
            (int64_t)gearData.ratio * drive->engineRpm) -
        drive->drivetrainTorque);
    ReadCarEngineTorque(drive, spec, gearData.torqueCurve,
                        &netTorque, &bandScale);
    UpdateCarGearShiftState(car, spec, &initialAcceleration);
    loads = CalculateCarDrivetrainLoads(
        car, spec, netTorque, bandScale, initialAcceleration);
    UpdateEngineRpm(drive, &loads);

    gearTorque = WrapSigned32(
        (int64_t)gearData.ratio * drive->engineRpm);
    drive->drivetrainTorque = gearTorque;
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        UpdateTakeoffSpeed(car, drive, loads.motionResistance);
    } else {
        UpdateDrivenSpeed(car, drive, spec, gearTorque);
    }

    AlignStoppedCarHeading(car);
    if (g_RacePhase >= 2) {
        DispatchCarMotion(car);
    } else {
        car->speed = 0;
    }
    AlignStoppedCarHeading(car);
}
