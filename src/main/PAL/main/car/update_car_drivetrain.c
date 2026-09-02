#include "game/state.h"
#include "game/angle.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track_internal.h"
#include "game/render.h"

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

/*
 * How much torque the engine makes right now, and how much of it engine
 * braking takes back.
 *
 * Past the rev limiter the torque goes negative in proportion to how far over
 * it is. Below it, two walks over the car's own curves: this gear's torque
 * band for the drive, and the loss curve for the braking, each interpolated
 * between the pair of points the engine speed falls between. First gear below
 * the redline gets twice the braking, which is what makes a car settle onto
 * the line instead of running wide.
 */
static s32 BandStartIndex(const s16 *bandEnds, s32 bandIndex) {
  s16 previousEnd;

  if (bandIndex == 0) {
    return 0;
  }
  previousEnd = bandEnds[bandIndex - 1];
  return previousEnd == 0 ? 0 : previousEnd - 1;
}

static s32 InterpolateDriveTorque(const GameCarSpec *spec,
                                  const s32 *gearCurve, s32 engineRpm,
                                  s32 bandIndex, s32 fallbackTorque) {
  s32 slot;
  s32 torque = fallbackTorque;

  for (slot = BandStartIndex(g_TorqueBandEnd, bandIndex);
       slot < g_TorqueBandEnd[bandIndex]; slot++) {
    s32 segmentStart = spec->torqueBand.values[slot];
    s32 segmentEnd = spec->torqueBand.values[slot + 1];
    s32 segmentLength;
    s32 weightedTorque;

    if (engineRpm < segmentStart || segmentEnd < engineRpm) {
      continue;
    }
    segmentLength = segmentEnd - segmentStart;
    if (segmentLength <= 0) {
      segmentLength = 1;
    }
    weightedTorque = (engineRpm - segmentStart) * gearCurve[slot + 1] +
                     (segmentEnd - engineRpm) * gearCurve[slot];
    torque = weightedTorque / (segmentLength * 0xA);
    break;
  }
  return torque < 0 ? 0 : torque;
}

static s32 InterpolateEngineBraking(const GameCarSpec *spec, s32 engineRpm,
                                    s32 bandIndex, s16 gear) {
  s32 slot;
  s32 braking = 0;

  for (slot = BandStartIndex(g_TorqueLossBandEnd, bandIndex);
       slot < g_TorqueLossBandEnd[bandIndex]; slot++) {
    s32 segmentStart = GetCarTorqueLossBoundary(spec, slot);
    s32 segmentEnd;
    s32 segmentLength;

    if (engineRpm < segmentStart) {
      continue;
    }
    segmentEnd = GetCarTorqueLossBoundary(spec, slot + 1);
    if (segmentEnd < engineRpm) {
      continue;
    }
    segmentLength = segmentEnd - segmentStart;
    if (segmentLength <= 0) {
      segmentLength = 1;
    }
    braking = ((engineRpm - segmentStart) *
                   spec->torqueLossValue[slot + 1] +
               (segmentEnd - engineRpm) *
                   spec->torqueLossValue[slot]) /
              segmentLength;
    break;
  }
  if (braking >= 0x64) {
    braking = 0x64;
  } else if (braking <= 0) {
    braking = 0;
  }
  if (gear == 1 && engineRpm < spec->redline) {
    braking *= 2;
  }
  return braking;
}

static void ReadEngineTorque(const GameCarDrive *drive,
                             const GameCarSpec *spec,
                             const s32 *gearCurve, s32 *netTorque,
                             s32 *bandScale) {
  s32 bandIndex;

  if (drive->engineRpm >= spec->revLimit) {
    *bandScale = 0;
    *netTorque = ((spec->revLimit - drive->engineRpm) * 4) / 5;
    return;
  }
  bandIndex = drive->engineRpm / 1000;
  if (bandIndex >= CAR_TORQUE_BAND_COUNT) {
    bandIndex = CAR_TORQUE_BAND_COUNT - 1;
  }
  *netTorque = InterpolateDriveTorque(
      spec, gearCurve, drive->engineRpm, bandIndex, *netTorque);
  *bandScale = InterpolateEngineBraking(
      spec, drive->engineRpm, bandIndex, drive->gear);
}

static s32 CalculateInitialAcceleration(const GameCarDrive *drive,
                                        s32 gearRatio) {
  s32 gearTorque = gearRatio * drive->engineRpm;
  s32 netLoad = gearTorque - drive->drivetrainTorque;
  s32 roundedLoad;

  if (drive->motionState == CAR_MOTION_TAKEOFF) {
    roundedLoad = netLoad < 0 ? netLoad + 0xFFF : netLoad;
    return roundedLoad >> 12;
  }
  if (netLoad < -0x30D3) {
    if (drive->motionState == CAR_MOTION_STANDING_START) {
      return netLoad / 768;
    }
    roundedLoad = netLoad + 0x7FF;
    return roundedLoad >> 11;
  }
  if (netLoad > 0x186A0) {
    roundedLoad = netLoad < 0 ? netLoad + 0xFF : netLoad;
    return ((roundedLoad >> 8) * 0x46) / 200;
  }
  return 0;
}

static void UpdateGearShiftState(PlayerCarRuntime *car, GameCarDrive *drive,
                                 const GameCarSpec *spec, s32 *acceleration) {
  s16 targetGear;
  s32 countdown;

  if (drive->motionState == CAR_MOTION_TAKEOFF ||
      drive->motionState == CAR_MOTION_STANDING_START) {
    drive->jumpTimer = 0;
    drive->clutch = 0;
    return;
  }

  if (drive->motionState == CAR_MOTION_AIRBORNE && drive->jumpTimer >= 0) {
    s16 nextTimer = drive->jumpTimer - 1;

    drive->jumpTimer = nextTimer < 0 ? 0 : nextTimer;
    *acceleration = 0;
    targetGear = drive->gear;
    if (drive->gearDisp != targetGear) {
      s32 targetRatio = GetPositiveCarGearRatio(spec, targetGear);
      s32 targetRpm = (((car->speed * 0xA0) / 1168) * 0x2710) /
                      targetRatio;
      u16 currentRpm = (u16)drive->engineRpm;

      g_ShiftTargetRpm = targetRpm;
      drive->shiftRpmDelta = (s16)((u16)targetRpm - currentRpm);
    }
    drive->engineRpm = drive->shiftRpmDelta * drive->jumpTimer / 20 +
                       g_ShiftTargetRpm;
    return;
  }

  targetGear = drive->gear;
  if (drive->gearDisp != targetGear) {
    s32 wheelSpeed = (u16)car->acceleration;
    s32 targetRatio = GetPositiveCarGearRatio(spec, targetGear);
    s32 targetSpeed = (car->speed * 0x2710) /
                      (targetRatio * 0x490 / 160);

    drive->engineLoad = wheelSpeed;
    g_ShiftTargetSpeed = targetSpeed;
    if (drive->manual != 0 && drive->gearDisp < targetGear &&
        g_RoadGrade < 0 && targetGear >= 4) {
      s32 gradePenalty;
      s32 gradeScale;

      if (targetGear == 4) {
        gradePenalty = -g_RoadGrade / 120;
      } else if (targetGear == 5) {
        gradePenalty = -g_RoadGrade / 48;
      } else {
        gradePenalty = g_RoadGrade * -7 / 240;
      }
      wheelSpeed = (s16)wheelSpeed;
      gradeScale = 0x64 - gradePenalty;
      drive->engineLoad =
          (u16)(wheelSpeed * gradeScale / 100);
      g_ShiftTargetSpeed = gradeScale * targetSpeed / 100;
    }

    *acceleration = 0;
    if (drive->gearDisp > targetGear) {
      g_ShiftTargetSpeed += 0x1F4;
    }
    drive->clutch = 0xA;
    drive->drivetrainCoupled = 0;
    drive->shiftSpeedDelta =
        (s16)((u16)g_ShiftTargetSpeed - (u16)drive->engineRpm);
    return;
  }

  countdown = --drive->clutch;
  if ((s16)countdown <= 0) {
    drive->drivetrainCoupled = 1;
    drive->engineLoad = 0;
    drive->clutch = 0;
  } else if (drive->manual != 0) {
    drive->engineRpm = g_ShiftTargetSpeed -
                       drive->shiftSpeedDelta * (s16)countdown / 15;
  } else {
    drive->engineRpm = g_ShiftTargetSpeed -
                       drive->shiftSpeedDelta * (s16)countdown / 10;
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

    initialAcceleration = CalculateInitialAcceleration(drive, gearRatio);
    /* If RPM falls between configured bands, retail keeps the raw wheel/load
     * difference rather than replacing it with an interpolated curve value. */
    netTorque = gearRatio * drive->engineRpm - drive->drivetrainTorque;
    ReadEngineTorque(drive, spec, gearCurve, &netTorque, &bandScale);
    UpdateGearShiftState(car, drive, spec, &initialAcceleration);
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
