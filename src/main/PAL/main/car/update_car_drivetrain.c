#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

enum {
    FIRST_FORWARD_GEAR = 1,
    MAX_FORWARD_GEAR = 6,
};

static s16 ClampDrivetrainGear(s16 gear) {
    if (gear < FIRST_FORWARD_GEAR) {
        return FIRST_FORWARD_GEAR;
    }
    if (gear > MAX_FORWARD_GEAR) {
        return MAX_FORWARD_GEAR;
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
        if (input >= 0x85) {
            *latch = 1;
        }
    } else if (*latch == 1) {
        *latch = 2;
    } else if (input < 0x7C) {
        *latch = 0;
    }
}

static void UpdateTakeoffSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                               s32 steeringResistance) {
    s32 brakeDrag = drive->brakeInput * 0x14;
    s32 coefficient = 0x26FC - 1 - steeringResistance * 2;
    s32 torque = drive->drivetrainTorque;

    if (brakeDrag < 0) {
        brakeDrag += 0xFF;
    }
    car->speed = (coefficient - (brakeDrag >> 8)) * car->speed / 10000;
    if (torque < 0) {
        torque += 0x1FFFFF;
    }
    car->acceleration = torque >> 21;
}

static void UpdateDrivenSpeed(PlayerCarRuntime *car, GameCarDrive *drive,
                              const GameCarSpec *spec, s32 gearTorque) {
    s32 speedScale;

    if (car->verticalMotionState != 0) {
        car->acceleration = 0;
        speedScale = 0x3E7;
        car->speed = car->speed * speedScale / 1000;
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
            : spec->automaticAccelerationScale * shiftedTorque / 1000;
    }
    if (g_GripLossTimer > 0) {
        car->acceleration /= 2;
    }
    car->speed = car->speed * 0x5E / 100;
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
    const s32 *gearCurve = g_GearTorqueCurve[gear].values;
    s32 gearRatio = GetCarGearLoad(spec, gear);
    s32 acceleratorGripNumerator;
    s32 acceleratorGripCost;
    s32 gripBudget;
    s32 initialAcceleration;
    s32 bandScale;
    s32 netTorque;
    s32 gearTorque;
    CarDrivetrainLoads loads;

    drive->gear = gear;
    if (g_RacePhase < 2) {
        drive->gearDisp = gear;
        gearRatio = spec->gearLoad[1];
        gearCurve = g_GearTorqueCurve[0].values;
    } else if (drive->motionState == CAR_MOTION_STANDING_START &&
               (drive->acceleratorInput.value < 0x40 ||
                drive->brakeInput >= 0x80)) {
        gearCurve = g_GearTorqueCurve[0].values;
    }

    LatchPedal(&drive->acceleratorLatch, drive->acceleratorInput.value);
    LatchPedal(&drive->brakeLatch, drive->brakeInput);
    acceleratorGripNumerator = drive->acceleratorInput.value * 0x64;
    acceleratorGripCost = acceleratorGripNumerator >> 8;
    if (acceleratorGripNumerator < 0) {
        acceleratorGripCost = (acceleratorGripNumerator + 0xFF) >> 8;
    }
    gripBudget = 0x17C - acceleratorGripCost;
    gripBudget += (drive->brakeInput * 0x64) / 256;
    UpdateCarSteeringGrip(car, spec, gripBudget);

    initialAcceleration = CalculateCarInitialAcceleration(drive, gearRatio);
    /* If RPM falls between configured bands, retail keeps the raw wheel/load
     * difference rather than replacing it with an interpolated curve value. */
    netTorque = gearRatio * drive->engineRpm - drive->drivetrainTorque;
    ReadCarEngineTorque(drive, spec, gearCurve, &netTorque, &bandScale);
    UpdateCarGearShiftState(car, spec, &initialAcceleration);
    loads = CalculateCarDrivetrainLoads(
        car, spec, netTorque, bandScale, initialAcceleration);
    if (drive->jumpTimer <= 0 && drive->clutch <= 0) {
        drive->engineRpm += loads.throttleAcceleration -
                            loads.accelerationResistance -
                            loads.steeringResistance;
    }
    if (drive->engineRpm < 0) {
        drive->engineRpm = 0;
    } else if (drive->engineRpm >= 0x3A99) {
        drive->engineRpm = 0x3A98;
    }

    gearTorque = gearRatio * drive->engineRpm;
    drive->drivetrainTorque = gearTorque;
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        UpdateTakeoffSpeed(car, drive, loads.steeringResistance);
    } else {
        UpdateDrivenSpeed(car, drive, spec, gearTorque);
    }

    if (car->speed < 8) {
        car->headingAngle = car->bodyYaw;
    }
    if (g_RacePhase >= 2) {
        DispatchCarMotion(car);
    } else {
        car->speed = 0;
    }
    if (car->speed < 8) {
        car->headingAngle = car->bodyYaw;
    }
}
