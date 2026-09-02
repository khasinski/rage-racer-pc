#include "game/state.h"
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

static s32 GetGearLoad(const GameCarSpec *spec, s16 gear) {
  /* Retail's sixth load is the adjacent gearRatio[0] word in the packed car
   * specification. Name that layout boundary instead of indexing gearLoad[6]. */
  return gear == MAX_FORWARD_GEAR ? spec->gearRatio[0]
                                  : spec->gearLoad[gear];
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
    s32 segmentStart = spec->torqueLossRpm[slot];
    s32 segmentEnd;
    s32 segmentLength;

    if (engineRpm < segmentStart) {
      continue;
    }
    segmentEnd = spec->torqueLossRpm[slot + 1];
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
  *netTorque = InterpolateDriveTorque(
      spec, gearCurve, drive->engineRpm, bandIndex, *netTorque);
  *bandScale = InterpolateEngineBraking(
      spec, drive->engineRpm, bandIndex, drive->gear);
}

/*
 * How much grip the steering has this frame.
 *
 * A car still on the ground works it out from the camber under it, and settles
 * halfway towards the answer rather than jumping to it. A car just off a crest
 * works it out from the curve instead: it carries the curve it thinks it is
 * on, the track point under it carries the curve it is really on, and agreeing
 * winds the bias up twice as fast as disagreeing unwinds it. A car on no curve
 * at all leaves the bias alone either way.
 *
 * Retail had a third path in the airborne case, for the two disagreeing with
 * the car on no curve and the point on no curve either, which cannot happen:
 * that is the two agreeing.
 */
static void UpdateSteeringGrip(PlayerCarRuntime *car, GameCarDrive *drive,
                               s32 gripBudget) {
    GameTrackPoint *trackPoint;
    s16 curveModeNow;
    s32 camber;
    s32 camberLean;

    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        s16 driveCurveMode = drive->trackCurveMode;
        s32 pointCurveMode = TrackPoint(car->trackPointIndex)->arcRef & 3;
        s16 steerBias;

        if (driveCurveMode != 0) {
            drive->trackCurveBias = (u16)drive->trackCurveBias +
                                    (driveCurveMode == pointCurveMode ? 2 : -1);
        }
        steerBias = drive->trackCurveBias;
        if (steerBias >= 0x1F) {
            drive->trackCurveBias = 0x1E;
        } else if (steerBias < -0x1E) {
            drive->trackCurveBias = -0x1E;
        }
        gripBudget += g_CarSpec->baseSteeringGrip - drive->trackCurveBias * 0xA;
        drive->steeringGrip = (s16)gripBudget;
        return;
    }
    trackPoint = TrackPoint(car->trackPointIndex);
    curveModeNow = drive->trackCurveMode;
    if (curveModeNow != (trackPoint->arcRef & 3) && curveModeNow != 0) {
        camber = trackPoint->crossSlope;
        if (camber < -0x32) {
            camber = -0x32;
        } else if (camber >= 0x33) {
            camber = 0x32;
        }
        /* The inside of a left-hander leans the other way. */
        camberLean = (trackPoint->arcRef & 3) == 1
            ? -(camber * 0x3C) / 20
            : camber * 3;
        gripBudget += camberLean;
    }
    drive->steeringGrip =
        (s16)((drive->steeringGrip +
               (gripBudget * drive->steeringGripResponse) / 1000) / 2);
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
      s32 targetRpm = (((car->speed * 0xA0) / 1168) * 0x2710) /
                      spec->gearRatio[targetGear];
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
    s32 targetSpeed = (car->speed * 0x2710) /
                      (spec->gearRatio[targetGear] * 0x490 / 160);

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

typedef struct DrivetrainLoads {
  s32 accelerationResistance;
  s32 steeringResistance;
  s32 throttleAcceleration;
} DrivetrainLoads;

static DrivetrainLoads CalculateDrivetrainLoads(
    PlayerCarRuntime *car, GameCarDrive *drive, s32 netTorque, s32 bandScale,
    s32 initialAcceleration) {
  DrivetrainLoads loads;
  s32 throttleTorque;
  s32 headingError;
  s32 trackHeadingError;
  s32 assistStep;
  s32 steerPosition;
  s32 pointIndex;
  s32 lateralOffset;
  s32 pitchSum;
  s32 roadGradeProduct;
  s32 roadGrade;
  s32 sideForce;
  s32 roadSpeed;
  s32 dragDivisor;

  loads.accelerationResistance = initialAcceleration;
  loads.steeringResistance = car->verticalMotionState == 0
      ? drive->engineRpm / 256
      : 0;

  throttleTorque = netTorque * drive->acceleratorInput.value *
                   drive->drivetrainCoupled;
  if (throttleTorque < 0) {
    throttleTorque += 0xFF;
  }
  loads.throttleAcceleration = throttleTorque >> 8;

  if (g_GripLossTimer > 0) {
    g_GripLossTimer--;
  } else {
    g_GripLossTimer = 0;
  }
  loads.accelerationResistance +=
      drive->brakeInput * drive->engineRpm / 8192;
  if (netTorque > 0) {
    if (drive->acceleratorInput.value < 0x7F) {
      loads.accelerationResistance += netTorque / 2;
    }
  } else {
    loads.accelerationResistance -= netTorque / 2;
  }

  headingError = GetAngleDistance(car->bodyYaw, car->headingAngle);
  drive->steeringLoadAngle = headingError < 0x401
      ? headingError
      : 0x800 - headingError;
  loads.steeringResistance += drive->steeringLoadAngle / 256;
  if (drive->motionState != CAR_MOTION_TAKEOFF && g_PadType == 0x41) {
    assistStep = g_CarSpec->negconSteeringAssistScale *
                 drive->steeringGripResponse / 1000;
    if (assistStep <= 0) {
      assistStep = 1;
    }
    steerPosition = drive->steerPos;
    if (steerPosition >= 0) {
      loads.steeringResistance += ((steerPosition * 5) / 6) / assistStep;
    } else {
      loads.steeringResistance -= ((steerPosition * 5) / 6) / assistStep;
    }
  }

  trackHeadingError = GetAngleDistance(
      car->headingAngle, 0xC00 - TrackPoint(car->trackPointIndex)->angle);
  pointIndex = car->trackPointIndex;
  lateralOffset = car->segmentFraction;
  pitchSum = TrackPoint(pointIndex)->surfacePitch *
             (0x400 - lateralOffset);
  pitchSum += TrackPoint((pointIndex + 1) % g_TrackPointCount)->surfacePitch *
              lateralOffset;
  if (pitchSum < 0) {
    pitchSum += 0x3FF;
  }
  roadGrade = pitchSum >> 10;
  roadGradeProduct = roadGrade * rcos(trackHeadingError);
  roadGrade = roadGradeProduct < 0
      ? (roadGradeProduct + 0xFFF) >> 12
      : roadGradeProduct >> 12;
  if (roadGrade < -0xEE) {
    roadGrade = -0xEE;
  } else if (roadGrade >= 0xEF) {
    roadGrade = 0xEE;
  }
  g_RoadGrade = roadGrade;
  sideForce = (-rsin(roadGrade)) * 0x708 / 0xA000;
  loads.steeringResistance += roadGrade < 0 ? sideForce : sideForce / 10;

  if (g_RacePhase == 2 &&
      drive->motionState == CAR_MOTION_STANDING_START) {
    loads.steeringResistance += (g_StandingStartSpin & 0x1F) * 5;
  }
  if (g_DriveBoostTimer > 0) {
    loads.steeringResistance += 0xC8 + g_DriveBoostTimer * 0x14;
    g_DriveBoostTimer--;
  }
  if (drive->motionState == CAR_MOTION_TAKEOFF) {
    loads.throttleAcceleration = loads.throttleAcceleration * 4 / 5;
  }

  roadSpeed = car->speed * 0xA0 / 1168;
  dragDivisor = g_CarSpec->speedDragDivisor * 0x3E8 /
                (s16)g_DragScale;
  if (dragDivisor <= 0) {
    dragDivisor = 1;
  }
  loads.steeringResistance += roadSpeed * roadSpeed / dragDivisor;
  g_DragScale = 0x3E8;
  if (car->verticalMotionState == 0) {
    loads.steeringResistance =
        loads.steeringResistance * (0x64 - bandScale) / 100;
  } else {
    loads.throttleAcceleration *= 2;
    loads.steeringResistance = 0;
  }
  return loads;
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
                              s32 gearTorque) {
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
        : g_CarSpec->automaticAccelerationScale * shiftedTorque / 1000;
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

void UpdateCarDrivetrain(PlayerCarRuntime *carArg) {
  const s32 *gearCurve;
  s16 gear;
  s32 gearTorqueLate;
  s32 frontLoad;
  s32 speedForPath;
  s32 frontLoadScaled;
  s32 gripBudget;
  s32 accel;
  s32 bandScale;
  s32 steerLoad;
  s32 throttleAccel;
  s32 gearRatio;
  s32 netTorque;
  GameCarDrive *drive;
  PlayerCarRuntime *car;
  const GameCarSpec *spec;
  DrivetrainLoads loads;
  car = carArg;
  spec = g_CarSpec;
  drive = &car->drive;
  gear = ClampDrivetrainGear(drive->gear);
  drive->gear = gear;
  gearCurve = g_GearTorqueCurve[gear].values;
  gearRatio = GetGearLoad(spec, gear);
  if (g_RacePhase < 2) {
    car->drive.gearDisp = gear;
    gearRatio = spec->gearLoad[1];
    gearCurve = g_GearTorqueCurve[0].values;
  } else if (car->drive.motionState == CAR_MOTION_STANDING_START &&
             (car->drive.acceleratorInput.value < 0x40 ||
              car->drive.brakeInput >= 0x80)) {
    gearCurve = g_GearTorqueCurve[0].values;
  }
  LatchPedal(&drive->acceleratorLatch, drive->acceleratorInput.value);
  LatchPedal(&drive->brakeLatch, drive->brakeInput);
  frontLoad = drive->acceleratorInput.value * 0x64;
  frontLoadScaled = frontLoad >> 8;
  if (frontLoad < 0) {
    frontLoadScaled = (frontLoad + 0xFF) >> 8;
  }
  gripBudget = 0x17C - frontLoadScaled;
  gripBudget += (drive->brakeInput * 0x64) / 256;
  UpdateSteeringGrip(car, drive, gripBudget);
  steerLoad = 0;
  accel = CalculateInitialAcceleration(drive, gearRatio);
  /* If RPM falls between configured bands, retail keeps the raw wheel/load
   * difference rather than replacing it with an interpolated curve value. */
  netTorque = gearRatio * drive->engineRpm - drive->drivetrainTorque;
  ReadEngineTorque(drive, spec, gearCurve, &netTorque, &bandScale);
  UpdateGearShiftState(car, drive, spec, &accel);
  loads = CalculateDrivetrainLoads(car, drive, netTorque, bandScale, accel);
  accel = loads.accelerationResistance;
  steerLoad = loads.steeringResistance;
  throttleAccel = loads.throttleAcceleration;
  if (drive->jumpTimer <= 0 && drive->clutch <= 0) {
    drive->engineRpm = throttleAccel - accel - steerLoad + drive->engineRpm;
  }
  speedForPath = drive->engineRpm;
  if (speedForPath < 0) {
    drive->engineRpm = 0;
  } else if (speedForPath >= 0x3A99) {
    drive->engineRpm = 0x3A98;
  }
  gearTorqueLate = gearRatio * drive->engineRpm;
  drive->drivetrainTorque = gearTorqueLate;
  if (drive->motionState == CAR_MOTION_TAKEOFF) {
    UpdateTakeoffSpeed(car, drive, steerLoad);
  } else {
    UpdateDrivenSpeed(car, drive, gearTorqueLate);
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
