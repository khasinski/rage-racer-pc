#include "common.h"
#include "game/state.h"
#include "game/track.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_control_command.h"
#include "game/car_internal.h"
#include "game/car_physics.h"
#include "game/track_internal.h"
#include "game/render.h"
#include "game/player_car_simulation.h"

void UpdateCarDrivetrain(PlayerCarRuntime *carArg,
                         const CarControlCommand *command,
                         const PlayerCarSimulationContext *simulation) {
  GearCurveAddress gearCurve;
  CarTorqueSample torqueSample;
  CarTransmissionState transmission;
  CarTransmissionInput transmissionInput;
  CarGroundSpeedInput groundSpeedInput;
  CarGroundSpeedOutput groundSpeedOutput;
  s16 curveModeNow;
  s16 revLimit;
  int assistArmed;
  int steeringNonnegative;
  s16 gear;
  s16 driveCurveMode;
  s16 steerBias;
  s32 camber;
  s32 shiftRemaining;
  s32 trackHeadingError;
  s32 pointIndex;
  s32 lateralOffset;
  s32 gearTorque;
  s32 dragProduct;
  s32 toCentreX;
  s32 gearTorqueLate;
  s32 cosCentreAngle;
  s32 toCentreZ;
  s32 centreAngle;
  s32 radialDistance;
  s32 pointCurveMode;
  s32 headingError;
  s32 sideForce;
  s32 roadSpeed;
  s32 arcPointIndex;
  s32 driveModeLate;
  s32 loadTorque;
  s32 driveMode;
  s32 frontLoadScaled;
  s32 downforceScale;
  s32 downforce;
  s32 gripBudget;
  s32 steeringAssistScale;
  s32 dragTerm;
  s32 slipAngle;
  s32 accel;
  s32 bandScale;
  s32 steerLoad;
  s32 throttleAccel;
  s32 gearRatio;
  s32 netTorque;
  s32 dragBase;
  s32 camberLean;
  s32 coefficientBase;
  s32 coefficient;
  u16 arcFlags;
  u16 steerBiasNext;
  GameCarDrive *drive;
  GameTrackArcCenter *arcCentre;
  GameTrackPoint *trackPoint;
  PlayerCarRuntime *car;
  GameCarSpecAddress config;
  GearCurveAddress base;
  car = carArg;
  base.rowPointer = (GearCurveRow *)simulation->gearTorqueCurves;
  config.pointer = (GameCarSpec *)simulation->carSpec;
  gear = car->drive.gear;
  gearCurve.valuePointer = base.rowPointer[gear].values;
  gearRatio = config.pointer->gearLoad[gear];
  drive = &car->drive;
  if (simulation->racePhase < 2)
  {
    car->drive.gearDisp = gear;
    gearRatio = config.pointer->gearLoad[1];
    gearCurve.valuePointer = base.rowPointer[0].values;
  }
  else
    if ((car->drive.motionState == CAR_MOTION_STANDING_START) && ((car->drive.acceleratorInput.value < 0x40) || (car->drive.brakeInput >= 0x80)))
  {
    gearCurve.valuePointer = base.rowPointer[0].values;
  }
  drive->acceleratorLatch = CarUpdatePedalLatch(
      drive->acceleratorLatch, drive->acceleratorInput.value);
  drive->brakeLatch = CarUpdatePedalLatch(
      drive->brakeLatch, drive->brakeInput);
  gripBudget = CarCalculateGripBudget(
      drive->acceleratorInput.value, drive->brakeInput);
  if (drive->motionState == CAR_MOTION_TAKEOFF)
  {
    driveCurveMode = drive->trackCurveMode;
    pointCurveMode = simulation->trackPoints[car->trackPointIndex].arcRef & 3;
    if (driveCurveMode != pointCurveMode)
    {
      if (driveCurveMode != 0)
      {
        steerBiasNext = (u16)drive->trackCurveBias - 1;
        goto block_29;
      }
      if (pointCurveMode == 0)
      {
        goto block_27;
      }
    }
    else
    {
      block_27:
      if (drive->trackCurveMode != 0)
      {
        steerBiasNext = (u16)drive->trackCurveBias + 2;
        block_29:
        drive->trackCurveBias = steerBiasNext;

      }

    }
    steerBias = drive->trackCurveBias;
    if (steerBias >= 0x1F)
    {
      drive->trackCurveBias = 0x1E;
    }
    else if (steerBias < (-0x1E))
    {
      drive->trackCurveBias = -0x1E;
    }
    gripBudget += simulation->carSpec->baseSteeringGrip - drive->trackCurveBias * 0xA;
    drive->steeringGrip = (s16)gripBudget;
  }
  else
  {
    trackPoint = (GameTrackPoint *)&simulation->trackPoints[car->trackPointIndex];
    curveModeNow = drive->trackCurveMode;
    if ((curveModeNow != (trackPoint->arcRef & 3)) && (curveModeNow != 0))
    {
      camber = trackPoint->crossSlope;
      if (camber < (-0x32))
      {
        camber = -0x32;
      }
      else
        if (camber >= 0x33)
      {
        camber = 0x32;
      }
      if ((simulation->trackPoints[car->trackPointIndex].arcRef & 3) == 1)
      {
        camberLean = (-(camber * 0x3C)) / 20;
      }
      else
      {
        camberLean = camber * 3;
      }
      gripBudget += camberLean;
    }
    drive->steeringGrip = (s16)((drive->steeringGrip + (gripBudget * drive->steeringGripResponse) / 1000) / 2);
  }
  gearTorque = gearRatio * drive->engineRpm;
  steerLoad = 0;
  loadTorque = drive->drivetrainTorque;
  netTorque = gearTorque - loadTorque;
  driveMode = drive->motionState;
  accel = CarCalculateLoadResistance(driveMode, gearTorque, loadTorque);
  revLimit = config.pointer->revLimit;
  torqueSample = CarSampleTorqueCurves(
      drive->engineRpm, revLimit, config.pointer->redline, drive->gear,
      netTorque, gearCurve.valuePointer, config.pointer->torqueBand.values,
      simulation->torqueBandEnd, config.pointer->torqueLossRpm,
      config.pointer->torqueLossValue, simulation->torqueLossBandEnd);
  netTorque = torqueSample.torque;
  bandScale = torqueSample.lossPercent;
  transmission = (CarTransmissionState){
      drive->motionState, drive->gear, drive->gearDisp, drive->jumpTimer,
      drive->clutch, drive->manual, drive->drivetrainCoupled,
      drive->shiftRpmDelta, drive->shiftSpeedDelta, drive->engineRpm,
      drive->engineLoad, *simulation->shiftTargetRpm,
      *simulation->shiftTargetSpeed};
  transmissionInput = (CarTransmissionInput){
      car->speed, car->acceleration, *simulation->roadGrade,
      config.pointer->gearRatio};
  if (CarUpdateTransmission(&transmission, &transmissionInput)) accel = 0;
  drive->jumpTimer = transmission.jumpTimer;
  drive->clutch = transmission.clutch;
  drive->drivetrainCoupled = transmission.drivetrainCoupled;
  drive->shiftRpmDelta = transmission.shiftRpmDelta;
  drive->shiftSpeedDelta = transmission.shiftSpeedDelta;
  drive->engineRpm = transmission.engineRpm;
  drive->engineLoad = transmission.engineLoad;
  *simulation->shiftTargetRpm = transmission.targetRpm;
  *simulation->shiftTargetSpeed = transmission.targetSpeed;
  throttleAccel = CarCalculateThrottleAcceleration(
      netTorque, drive->acceleratorInput.value, drive->drivetrainCoupled);
  if (*simulation->gripLossTimer > 0)
  {
    *simulation->gripLossTimer -= 1;
  }
  else
  {
    *simulation->gripLossTimer = 0;
  }
  if (car->shiftState == 0)
  {
    steerLoad += drive->engineRpm / 256;
  }
  accel += drive->brakeInput * drive->engineRpm / 8192;
  if (netTorque > 0)
  {
    if (drive->acceleratorInput.value < 0x7F)
    {
      accel += netTorque / 2;
    }
  }
  else
  {
    accel -= netTorque / 2;
  }
  headingError = GetAngleDistance(car->bodyYaw, car->headingAngle);
  drive->steeringLoadAngle = headingError;
  if (headingError >= 0x401)
  {
    drive->steeringLoadAngle = 0x800 - headingError;
  }
  steerLoad += drive->steeringLoadAngle / 256;
  if ((drive->motionState != CAR_MOTION_TAKEOFF) && command->digitalController)
  {
    steeringAssistScale = simulation->carSpec->negconSteeringAssistScale * drive->steeringGripResponse / 1000;
    if (steeringAssistScale <= 0)
    {
      steeringAssistScale = 1;
    }
    shiftRemaining = drive->steerPos;
    assistArmed = shiftRemaining >= 0;
    if (assistArmed)
    {
      steerLoad += ((shiftRemaining * 5) / 6) / steeringAssistScale;
    }
    else
    {
      steerLoad -= ((shiftRemaining * 5) / 6) / steeringAssistScale;
    }
  }
  trackHeadingError = GetAngleDistance(car->headingAngle,
                                       0xC00 - simulation->trackPoints[car->trackPointIndex].angle);
  frontLoadScaled = trackHeadingError;
  pointIndex = car->trackPointIndex;
  lateralOffset = car->segmentFraction;
  pointIndex += 1;
  slipAngle = CarInterpolateSurfacePitch(
      simulation->trackPoints[car->trackPointIndex].surfacePitch,
      simulation->trackPoints[pointIndex % simulation->trackPointCount].surfacePitch,
      lateralOffset);
  dragProduct = slipAngle * rcos(frontLoadScaled);
  slipAngle = dragProduct >> 0xC;
  if (dragProduct < 0)
  {
    slipAngle = (dragProduct + 0xFFF) >> 0xC;
  }
  if (slipAngle < (-0xEE))
  {
    slipAngle = -0xEE;
  }
  else
    if (slipAngle >= 0xEF)
  {
    slipAngle = 0xEE;
  }
  sideForce = (-rsin(slipAngle)) * 0x708;
  *simulation->roadGrade = slipAngle;
  frontLoadScaled = sideForce / 0xA000;
  steeringNonnegative = slipAngle >= 0;
  if (!steeringNonnegative)
  {
    steerLoad += frontLoadScaled;
  }
  else
  {
    steerLoad += frontLoadScaled / 10;
  }
  if ((simulation->racePhase == 2) && (drive->motionState == CAR_MOTION_STANDING_START))
  {
    steerLoad += (simulation->standingStartSpin & 0x1F) * 5;
  }
  {
    s32 counter = *simulation->driveBoostTimer;
    if (counter > 0)
    {
      s32 baseValue = steerLoad + 0xC8;
      steerLoad = baseValue + (counter * 0x14);
      *simulation->driveBoostTimer = counter - 1;
    }
  }
  if (drive->motionState == CAR_MOTION_TAKEOFF)
  {
    throttleAccel = (throttleAccel * 4) / 5;
  }
  roadSpeed = car->speed * 0xA0 / 1168;
  dragBase = simulation->carSpec->speedDragDivisor * 0x3E8;
  dragTerm = dragBase / *simulation->dragScale;
  if (dragTerm <= 0)
  {
    dragTerm = 1;
  }
  steerLoad += (roadSpeed * roadSpeed) / dragTerm;
  *simulation->dragScale = 0x3E8;
  if (car->shiftState == 0)
  {
    steerLoad = (steerLoad * (0x64 - bandScale)) / 100;
  }
  else
  {
    throttleAccel *= 2;
    steerLoad = 0;
  }
  drive->engineRpm = CarIntegrateEngineRpm(
      drive->engineRpm, throttleAccel, accel, steerLoad,
      drive->jumpTimer, drive->clutch);
  gearTorqueLate = gearRatio * drive->engineRpm;
  drive->drivetrainTorque = gearTorqueLate;
  if (drive->motionState == CAR_MOTION_TAKEOFF)
  {
    arcPointIndex = car->trackPointIndex;
    arcFlags = simulation->trackPoints[arcPointIndex].arcRef;
    dragBase = arcFlags % 4;
    if (dragBase > 0)
    {
      arcCentre = (GameTrackArcCenter *)&simulation->trackArcCenters[(s16)arcFlags >> 4];
      toCentreX = car->x - arcCentre->x;
      toCentreZ = car->z - arcCentre->z;
      centreAngle = Atan2(toCentreX, toCentreZ);
      cosCentreAngle = rcos(centreAngle);
      radialDistance = (cosCentreAngle * toCentreX) + (rsin(centreAngle) * toCentreZ);
      frontLoadScaled = radialDistance >> 0xC;
      if (radialDistance < 0)
      {
        frontLoadScaled = (radialDistance + 0xFFF) >> 0xC;
      }
    }
    else
    {
      frontLoadScaled = simulation->carSpec->referenceTurnRadius * 0x64;
    }
    if ((frontLoadScaled <= 0) || ((downforceScale = simulation->carSpec->referenceTurnRadius * 0x64, downforceScale <= 0)))
    {
      downforceScale = simulation->carSpec->referenceTurnRadius * 0x64;
    }
    downforce = (simulation->carSpec->referenceTurnRadius * 0x64) / downforceScale;
    if (downforce <= 0)
    {
      downforce = 1;
    }
    dragTerm = drive->brakeInput * 0x14;
    coefficientBase = 0x26FC - downforce;
    coefficient = coefficientBase - (steerLoad * 2);
    if (dragTerm < 0)
    {
      dragTerm += 0xFF;
    }
    car->speed = (coefficient - (dragTerm >> 8)) * car->speed / 10000;
    arcPointIndex = drive->drivetrainTorque;
    if (arcPointIndex < 0)
    {
      arcPointIndex += 0x1FFFFF;
    }
    dragBase = arcPointIndex >> 0x15;
    car->acceleration = dragBase;
  }
  else
  {
    groundSpeedInput = (CarGroundSpeedInput){
        car->speed, gearTorqueLate, drive->engineLoad,
        simulation->carSpec->automaticAccelerationScale, car->shiftState,
        drive->clutch, drive->jumpTimer, drive->manual,
        *simulation->gripLossTimer > 0};
    groundSpeedOutput = CarCalculateGroundSpeed(&groundSpeedInput);
    car->speed = groundSpeedOutput.speed;
    car->acceleration = groundSpeedOutput.acceleration;
  }
  if (car->speed < 8)
  {
    car->headingAngle = car->bodyYaw;
  }
  if (simulation->racePhase >= 2)
  {
    driveModeLate = drive->motionState;
    switch (driveModeLate)
    {
      case 0:
        UpdateCarDriving(car);
        break;

      case 1:
        UpdateCarLaunch(car);
        break;

      case 2:
        UpdateCarAirborne(car);
        break;

      case 3:
        UpdateCarStandingStart(car);
        break;

    }

  }
  else
  {
    car->speed = 0;
  }
  if (car->speed < 8)
  {
    car->headingAngle = car->bodyYaw;
  }
}
