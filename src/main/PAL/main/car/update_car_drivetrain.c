#include "common.h"
#include "game/game_input.h"
#include "game/state.h"
#include "game/track.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/car_physics.h"
#include "game/track_internal.h"
#include "game/render.h"

typedef union DrivetrainWheelSpeed {
  s32 value;
  u32 unsignedValue;
} DrivetrainWheelSpeed;

/*
 * Note on `gearCurve`: m2c merged two values into one temporary, so it starts
 * out as the per-gear torque curve pointer and is later reused to carry the
 * shift target speed. Splitting it changes the register assignment.
 */
void UpdateCarDrivetrain(PlayerCarRuntime *carArg) {
  GearCurveAddress gearCurve;
  s16 curveModeNow;
  s16 revLimit;
  s16 targetGear;
  s32 bandEnd;
  s16 bandStart;
  s16 lossStart;
  s16 targetGearAgain;
  int assistArmed;
  int steeringNonnegative;
  int secondNonnegative;
  s16 shiftTimer;
  s16 shiftTimerNext;
  s32 assistEnabled;
  s16 gear;
  s16 targetGearCheck;
  s16 driveCurveMode;
  s16 steerBias;
  s32 camber;
  s32 bandTorque;
  s32 lossTorque;
  s32 shiftRemaining;
  s32 trackHeadingError;
  s32 pointIndex;
  s32 wheelSpeed;
  s32 lateralOffset;
  s32 gearTorque;
  s32 dragProduct;
  s32 toCentreX;
  s32 gearTorqueLate;
  s32 cosCentreAngle;
  s32 toCentreZ;
  s32 engineSpeed;
  s32 engineSpeedLoss;
  s32 bandIndex;
  s32 speedForPath;
  s32 centreAngle;
  s32 radialDistance;
  s32 shiftTargetRpm;
  s32 pointCurveMode;
  s32 headingError;
  s32 shiftMode;
  s32 gradeScale;
  s32 sideForce;
  s32 roadSpeed;
  s32 arcPointIndex;
  s32 speedA;
  s32 torqueShifted;
  s32 speedB;
  s32 driveModeLate;
  s32 loadTorque;
  s32 driveMode;
  s32 frontLoadScaled;
  s32 shiftTargetSpeed;
  s32 downforceScale;
  s32 downforce;
  s32 gripBudget;
  s32 bandSlot;
  s32 bandCurve;
  s32 assistStep;
  int shiftTimerActive;
  s32 lossCurve;
  s32 dragTerm;
  s32 slipAngle;
  s32 accel;
  s32 bandScale;
  s32 steerLoad;
  s32 throttleAccel;
  s32 gearRatio;
  s32 *gearRatios;
  s32 netTorque;
  s32 gradePenalty;
  s32 lateralSum;
  s32 dragBase;
  s32 camberLean;
  s32 netTorqueRoundedA;
  s32 netTorqueRoundedC;
  s32 lossBelowLimit;
  s32 shiftedSpeed;
  s32 netTorqueRoundedB;
  s32 bandBase;
  s32 lossBase;
  s32 throttleTorque;
  s32 speedScaled;
  s32 torqueLate;
  s32 coefficientBase;
  s32 coefficient;
  DrivetrainWheelSpeed wheelSpeedScaled;
  u16 arcFlags;
  u16 currentSpeed;
  u16 steerBiasNext;
  GameCarDrive *drive;
  GameTrackArcCenter *arcCentre;
  GameTrackPoint *trackPoint;
  GearCurveAddress curveSlot;
  PlayerCarRuntime *car;
  GameCarSpecAddress config;
  GearCurveAddress base;
  car = carArg;
  base.rowPointer = g_GearTorqueCurve;
  config.pointer = g_CarSpec;
  gear = car->drive.gear;
  gearCurve.valuePointer = base.rowPointer[gear].values;
  gearRatio = config.pointer->gearLoad[gear];
  drive = &car->drive;
  if (g_RacePhase < 2)
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
    pointCurveMode = g_TrackPoints[car->trackPointIndex].arcRef & 3;
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
    gripBudget += g_CarSpec->baseSteeringGrip - drive->trackCurveBias * 0xA;
    drive->steeringGrip = (s16)gripBudget;
  }
  else
  {
    trackPoint = &g_TrackPoints[car->trackPointIndex];
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
      if ((g_TrackPoints[car->trackPointIndex].arcRef & 3) == 1)
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
  accel = 0;
  if (driveMode == 1)
  {
    netTorqueRoundedA = netTorque;
    if (netTorque < 0)
    {
      netTorqueRoundedA = netTorque + 0xFFF;
    }
    accel = netTorqueRoundedA;
    accel = accel >> 0xC;
  }
  else
    if (netTorque >= (-0x30D3))
  {
    if (netTorque > 0x186A0)
    {
      netTorqueRoundedB = netTorque;
      if (netTorque < 0)
      {
        netTorqueRoundedB = netTorque + 0xFF;
      }
      accel = ((netTorqueRoundedB >> 8) * 0x46) / 200;
    }
  }
  else
    if (driveMode == 3)
  {
    accel = (gearTorque - loadTorque) / 768;
  }
  else
  {
    netTorqueRoundedC = netTorque;
    if (netTorque < 0)
    {
      netTorqueRoundedC = netTorque + 0x7FF;
    }
    accel = netTorqueRoundedC >> 0xB;
  }
  revLimit = config.pointer->revLimit;
  if (drive->engineRpm >= revLimit)
  {
    bandScale = 0;
    netTorque = ((revLimit - drive->engineRpm) * 4) / 5;
  }
  else
  {
    bandIndex = drive->engineRpm / 1000;
    if (bandIndex == 0)
    {
      bandBase = 0;
    }
    else
    {
      /* Retail's start symbol is the halfword immediately before the end
       * table: start[b] aliases b == 0 to the standalone halfword and b > 0
       * to end[b - 1]. Independently linked host globals are not adjacent. */
      bandStart = g_TorqueBandEnd[bandIndex - 1];
      if (bandStart == 0)
      {
        bandBase = 0;
      }
      else
      {
        bandBase = bandStart - 1;
      }
    }
    bandEnd = g_TorqueBandEnd[bandIndex];
    bandSlot = bandBase;
    if (bandSlot < bandEnd)
    {
      engineSpeed = drive->engineRpm;
      loop_68:
      bandTorque = config.pointer->torqueBand.values[bandSlot];

      if ((engineSpeed >= bandTorque) &&
          (config.pointer->torqueBand.values[bandSlot + 1] >= engineSpeed))
      {
        s32 *curveValues = &gearCurve.valuePointer[bandSlot];
        s32 bandNext = config.pointer->torqueBand.values[bandSlot + 1];
        bandCurve = bandNext - bandTorque;
        if (bandCurve <= 0)
        {
          bandCurve = 1;
        }
        frontLoadScaled = (engineSpeed - bandTorque) * curveValues[1];
        frontLoadScaled += (bandNext - engineSpeed) * curveValues[0];
        netTorque = frontLoadScaled / (bandCurve * 0xA);
      }
      else
      {
        bandSlot += 1;
        if (bandSlot < bandEnd)
        {
          goto loop_68;
        }
      }
    }
    if (netTorque < 0)
    {
      netTorque = 0;
    }
    if (bandIndex == 0)
    {
      lossBase = 0;
    }
    else
    {
      /* Same split-symbol layout as the torque-band table above. */
      lossStart = g_TorqueLossBandEnd[bandIndex - 1];
      lossBase = 0;
      if (lossStart != 0)
      {
        lossBase = lossStart - 1;
      }
    }
    bandEnd = g_TorqueLossBandEnd[bandIndex];
    assistStep = lossBase;
    bandScale = 0;
    if (assistStep < bandEnd)
    {
      engineSpeedLoss = drive->engineRpm;
      loop_83:
      lossTorque = config.pointer->torqueLossRpm[assistStep];

      if (engineSpeedLoss >= lossTorque)
      {
        curveSlot.value = config.pointer->torqueLossRpm[assistStep + 1];
        assistStep += 1;
        if (curveSlot.value >= engineSpeedLoss)
        {
          lossCurve = curveSlot.value - lossTorque;
          if (lossCurve <= 0)
          {
            lossCurve = 1;
          }
          bandScale = (((engineSpeedLoss - lossTorque) * config.pointer->torqueLossValue[assistStep]) + ((curveSlot.value - engineSpeedLoss) * config.pointer->torqueLossValue[assistStep - 1])) / lossCurve;
          lossBelowLimit = bandScale < 0x64;
        }
        else
        {
          goto block_89;
        }
      }
      else
      {
        assistStep = assistStep + 1;
        block_89:
        if (assistStep >= bandEnd)
        {
          goto block_90;
        }
        goto loop_83;
      }
    }
    else
    {
      block_90:
      lossBelowLimit = bandScale < 0x64;

    }
    if (lossBelowLimit == 0)
    {
      bandScale = 0x64;
    }
    else
      if (bandScale <= 0)
    {
      bandScale = 0;
    }
    if ((drive->gear == 1) && (drive->engineRpm < g_CarSpec->redline))
    {
      bandScale *= 2;
    }
  }
  shiftMode = drive->motionState;
  if ((shiftMode == 1) || (shiftMode == 3))
  {
    drive->jumpTimer = 0;
    drive->clutch = 0;
  }
  else
  {
    if (shiftMode == 2)
    {
      shiftTimer = drive->jumpTimer;
      shiftTimerActive = shiftTimer >= 0;
      if (shiftTimerActive)
      {
        shiftTimerNext = shiftTimer - 1;
        drive->jumpTimer = shiftTimerNext;
        accel = 0;
        if (shiftTimerNext < 0)
        {
          drive->jumpTimer = 0;
        }
        targetGear = drive->gear;
        if (drive->gearDisp != targetGear)
        {
          gearRatios = g_CarSpec->gearRatio;
          shiftTargetRpm = (((car->speed * 0xA0) / 1168) * 0x2710) /
                           gearRatios[targetGear];
          currentSpeed = (u16)drive->engineRpm;
          g_ShiftTargetRpm = shiftTargetRpm;
          drive->shiftRpmDelta = (s16)((u16)g_ShiftTargetRpm - currentSpeed);
        }
        bandEnd = drive->shiftRpmDelta * drive->jumpTimer / 20;
        shiftedSpeed = bandEnd;
        shiftedSpeed = shiftedSpeed + g_ShiftTargetRpm;
        goto block_129;
      }
    }
    targetGearAgain = drive->gear;
    if (drive->gearDisp != targetGearAgain)
    {
      gearRatios = config.pointer->gearRatio;
      gearCurve.value = (car->speed * 0x2710) /
                            (gearRatios[targetGearAgain] * 0x490 / 160);
      wheelSpeed = (u16)car->acceleration;
      wheelSpeedScaled.value = wheelSpeed;
      assistEnabled = drive->manual;
      drive->engineLoad = wheelSpeedScaled.value;
      g_ShiftTargetSpeed = gearCurve.value;
      if (assistEnabled != 0)
      {
        targetGearCheck = drive->gear;
        if ((drive->gearDisp < targetGearCheck) && (g_RoadGrade < 0))
        {
          if (targetGearCheck < 4)
          {
            goto grade_adjust_done;
          }
          if (targetGearCheck == 4)
          {
            gradePenalty = (-g_RoadGrade) / 120;
      wheelSpeedScaled.unsignedValue <<= 16;
            wheelSpeedScaled.value >>= 16;
          }
          else
            if (targetGearCheck == 5)
          {
            gradePenalty = (-g_RoadGrade) / 48;
      wheelSpeedScaled.unsignedValue <<= 16;
            wheelSpeedScaled.value >>= 16;
          }
          else
            if (targetGearCheck >= 6)
          {
            gradePenalty = (g_RoadGrade * (-7)) / 240;
        wheelSpeedScaled.unsignedValue <<= 16;
            wheelSpeedScaled.value >>= 16;
          }
          else
          {
            goto grade_adjust_done;
          }
          gradeScale = 0x64 - gradePenalty;
          drive->engineLoad = (u16)((wheelSpeedScaled.value * gradeScale) / 100);
          g_ShiftTargetSpeed = (gradeScale * gearCurve.value) / 100;
        }
      }
grade_adjust_done:
      shiftTargetSpeed = g_ShiftTargetSpeed;

      accel = 0;
      if (drive->gearDisp > drive->gear)
      {
        shiftTargetSpeed += 0x1F4;
      }
      g_ShiftTargetSpeed = shiftTargetSpeed;
      {
        u16 targetSpeed = (u16) g_ShiftTargetSpeed;
        u16 currentSpeed = (u16)drive->engineRpm;
        drive->clutch = 0xA;
        drive->drivetrainCoupled = 0;
        drive->shiftSpeedDelta = (s16)(targetSpeed - currentSpeed);
      }
    }
    else
    {
      {
        s32 countdown = --drive->clutch;
      if (((s16) countdown) <= 0)
      {
        drive->drivetrainCoupled = 1;
        drive->engineLoad = 0;
        drive->clutch = 0;
      }
      else
        if (drive->manual != 0)
      {
        drive->engineRpm = g_ShiftTargetSpeed - drive->shiftSpeedDelta * (s16)countdown / 15;
      }
      else
      {
        shiftRemaining = drive->shiftSpeedDelta * (s16)countdown;
        lossBase = shiftRemaining / 10;
        drive->engineRpm = g_ShiftTargetSpeed - lossBase;
        goto shift_interpolation_done;
        block_129:
        drive->engineRpm = shiftedSpeed;
shift_interpolation_done:

      }
      }
    }
  }
  throttleTorque = netTorque * drive->acceleratorInput.value * drive->drivetrainCoupled;
  if (throttleTorque < 0)
  {
    throttleTorque += 0xFF;
  }
  throttleAccel = throttleTorque >> 8;
  if (g_GripLossTimer > 0)
  {
    g_GripLossTimer -= 1;
  }
  else
  {
    g_GripLossTimer = 0;
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
  if ((drive->motionState != CAR_MOTION_TAKEOFF) && (g_GameInput.controllerType == 0x41))
  {
    assistStep = g_CarSpec->negconSteeringAssistScale * drive->steeringGripResponse / 1000;
    if (assistStep <= 0)
    {
      assistStep = 1;
    }
    shiftRemaining = drive->steerPos;
    assistArmed = shiftRemaining >= 0;
    if (assistArmed)
    {
      steerLoad += ((shiftRemaining * 5) / 6) / assistStep;
    }
    else
    {
      steerLoad -= ((shiftRemaining * 5) / 6) / assistStep;
    }
  }
  trackHeadingError = GetAngleDistance(car->headingAngle,
                                       0xC00 - g_TrackPoints[car->trackPointIndex].angle);
  frontLoadScaled = trackHeadingError;
  pointIndex = car->trackPointIndex;
  lateralOffset = car->segmentFraction;
  engineSpeed = g_TrackPoints[pointIndex].surfacePitch * (0x400 - lateralOffset);
  pointIndex += 1;
  lateralSum = engineSpeed +
               g_TrackPoints[pointIndex % g_TrackPointCount].surfacePitch * lateralOffset;
  secondNonnegative = lateralSum >= 0;
  if (!secondNonnegative)
  {
    lateralSum += 0x3FF;
  }
  slipAngle = lateralSum >> 0xA;
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
  g_RoadGrade = slipAngle;
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
  if ((g_RacePhase == 2) && (drive->motionState == CAR_MOTION_STANDING_START))
  {
    steerLoad += (g_StandingStartSpin & 0x1F) * 5;
  }
  {
    s32 counter = g_DriveBoostTimer;
    if (counter > 0)
    {
      s32 baseValue = steerLoad + 0xC8;
      steerLoad = baseValue + (counter * 0x14);
      g_DriveBoostTimer = counter - 1;
    }
  }
  if (drive->motionState == CAR_MOTION_TAKEOFF)
  {
    throttleAccel = (throttleAccel * 4) / 5;
  }
  shiftTargetSpeed = (roadSpeed = car->speed * 0xA0 / 1168);
  dragBase = g_CarSpec->speedDragDivisor * 0x3E8;
  dragTerm = dragBase / ((s16) g_DragScale);
  if (dragTerm <= 0)
  {
    dragTerm = 1;
  }
  steerLoad += (roadSpeed * roadSpeed) / dragTerm;
  g_DragScale = 0x3E8;
  if (car->shiftState == 0)
  {
    steerLoad = (steerLoad * (0x64 - bandScale)) / 100;
  }
  else
  {
    throttleAccel *= 2;
    steerLoad = 0;
  }
  if ((drive->jumpTimer <= 0) && (drive->clutch <= 0))
  {
    drive->engineRpm = throttleAccel - accel - steerLoad + drive->engineRpm;
  }
  speedForPath = drive->engineRpm;
  if (speedForPath < 0)
  {
    drive->engineRpm = 0;
  }
  else
    if (speedForPath >= 0x3A99)
  {
    drive->engineRpm = 0x3A98;
  }
  gearTorqueLate = gearRatio * drive->engineRpm;
  drive->drivetrainTorque = gearTorqueLate;
  if (drive->motionState == CAR_MOTION_TAKEOFF)
  {
    arcPointIndex = car->trackPointIndex;
    arcFlags = g_TrackPoints[arcPointIndex].arcRef;
    dragBase = arcFlags % 4;
    if (dragBase > 0)
    {
      arcCentre = &g_TrackArcCenters[(s16)arcFlags >> 4];
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
      frontLoadScaled = (g_CarSpec->referenceTurnRadius) * 0x64;
    }
    if ((frontLoadScaled <= 0) || ((downforceScale = (g_CarSpec->referenceTurnRadius) * 0x64, downforceScale <= 0)))
    {
      downforceScale = (g_CarSpec->referenceTurnRadius) * 0x64;
    }
    downforce = (g_CarSpec->referenceTurnRadius * 0x64) / downforceScale;
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
    if (car->shiftState != 0)
    {
      speedA = car->speed;
      car->acceleration = 0;
      speedScaled = (speedA * 0x3E7) / 1000;
    }
    else
    {
      if (drive->clutch > 0)
      {
        car->acceleration = drive->engineLoad;
      }
      else
      {
        torqueLate = gearTorqueLate;
        if (drive->jumpTimer > 0)
        {
          car->acceleration = drive->engineLoad;
        }
        else
        {
          if (torqueLate < 0)
          {
            torqueLate += 0x1FFFF;
          }
          torqueShifted = torqueLate >> 0x11;
          car->acceleration = torqueShifted;
          if (drive->manual == 0)
          {
            car->acceleration = g_CarSpec->automaticAccelerationScale * torqueShifted / 1000;
          }
        }
      }
      if (g_GripLossTimer > 0)
      {
        car->acceleration /= 2;
      }
      speedB = car->speed;
      speedScaled = (speedB * 0x5E) / 100;
    }
    car->speed = speedScaled;
  }
  if (car->speed < 8)
  {
    car->headingAngle = car->bodyYaw;
  }
  if (g_RacePhase >= 2)
  {
    driveModeLate = drive->motionState;
    switch (driveModeLate)
    {
      case 0:
        UpdateCarDriving(car, dragTerm);
        break;

      case 1:
        UpdateCarLaunch(car, dragTerm);
        break;

      case 2:
        UpdateCarAirborne(car, dragTerm);
        break;

      case 3:
        UpdateCarStandingStart(car, dragTerm);
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
