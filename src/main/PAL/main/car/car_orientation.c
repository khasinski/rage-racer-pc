#include "common.h"
#include "game/state.h"
#include <stdio.h>
#include "game/vector.h"
#include "game/track.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/render.h"
#include "psyq/gte.h"
#include "game/audio.h"

void InitPlayerCar(PlayerCarRuntime *car)
{
  CarTrackLimits trackState;
  int scaledGearRatio;
  Matrix rotationMatrix;
  Matrix axisMatrix;
  SVec rotationOffset;
  PlayerCarRuntime *player;
  GameCarDrive *drive;
  TrackEventData *eventData;
  s32 speedBandOffset;
  s32 i;
  s32 headingBase;
  s32 divisor;
  s32 speedThreshold;
  s16 *revLimitPtr;
  s32 peakRpm;
  s32 value;
  s32 j;
  s32 k;
  GameCarSpec *carSpec;
  GameCarSpec *curveSpec;
  s16 *torqueBand;
  s16 *accelBand;
  s16 *accelBandOut;
  s32 bandSpeed;
  player = car;
  eventData = g_TrackEventData;
  printf(g_MsgInitCar);
  value = g_GrandPrixSeries;
  g_RacePhase = 2;
  g_RaceSeries = value & 1;
  BuildTachoNeedleQuad();
  ClearCarMotionState(car);
  g_AutoShiftCooldown = 0;
  g_TrackZoneDark = 0;
  g_ShiftSoundLevel = 0;
  g_RoadGrade = 0;
  player->modelIndex = 0x17;
  player->drive.brakePos = 0;
  player->drive.reserved0C = 0;
  player->drive.accelPos = 0;
  player->drive.reserved20 = 0;
  player->drive.steerPos = 0;
  player->drive.reserved18 = 0;
  player->motionZ = 0;
  player->motionY = 0;
  player->motionX = 0;
  player->wheelRotation = 0;
  player->steeringAngle = 0;
  player->reserved40 = 0;
  player->speed = 0;
  player->acceleration = 0;
  player->lap = 0;
  player->drive.bodyLiftOffset = 0;
  player->progressA = 0;
  player->progressB = 0;
  player->trackProgress = 0;
  printf(g_MsgHTbl);
  player->trackPointIndex = eventData->rivalStarts[ReadStableRaceSeries()][0].trackPointIndex;
  player->x = eventData->rivalStarts[ReadStableRaceSeries()][0].x;
  player->z = eventData->rivalStarts[ReadStableRaceSeries()][0].z;
  player->y = 0;
  player->trackPointIndex = FindTrackSegment(car, player->trackPointIndex);
  player->bodyPitch = 0;
  headingBase = 0xC00 - (ReadStableRaceSeries() << 11);
  player->bodyYaw = (headingBase - g_TrackPoints[player->trackPointIndex].angle) & 0xFFF;
  player->bodyRoll = 0;
  player->bodyRollVelocity = 0;
  player->previousTrackPointIndex = player->trackPointIndex;
  player->headingAngle = player->bodyYaw;
  player->drive.targetHeading = player->headingAngle;
  SeedCarLapProgress(GetPlayerCarRuntime(car), 0);
  trackState.rightInset = 0;
  trackState.leftInset = 0;
  UpdateCarTrackState(car, player->trackPointIndex, &trackState);
  player->previousTrackProgress = player->trackProgress;
  CopyPlayerBodyRotationToModel(player);
  player->modelY = player->y;
  BuildRotMatrixY(&rotationMatrix, player->bodyYaw);
  BuildRotMatrixX(&axisMatrix, player->bodyPitch);
  MulMatrix2(&axisMatrix, &rotationMatrix);
  BuildRotMatrixZ(&axisMatrix, player->bodyRoll);
  MulMatrix2(&axisMatrix, &rotationMatrix);
  rotationOffset.vx = 0;
  rotationOffset.vy = 0;
  axisMatrix.m[0][0] = rotationMatrix.m[0][0];
  axisMatrix.m[0][1] = rotationMatrix.m[1][0];
  axisMatrix.m[0][2] = rotationMatrix.m[2][0];
  axisMatrix.m[1][0] = rotationMatrix.m[0][1];
  axisMatrix.m[1][1] = rotationMatrix.m[1][1];
  axisMatrix.m[1][2] = rotationMatrix.m[2][1];
  axisMatrix.m[2][0] = rotationMatrix.m[0][2];
  axisMatrix.m[2][1] = rotationMatrix.m[1][2];
  axisMatrix.m[2][2] = rotationMatrix.m[2][2];
  rotationOffset.vz = -player->drive.bodyLiftOffset - 0x32;
  ApplyMatrix(&axisMatrix, &rotationOffset, &player->motionX);
  player->drive.hudLapHighlightRow = -1;
  player->drive.motionState = CAR_MOTION_STANDING_START;
  player->drive.engineLoad = 0;
  player->drive.drivetrainCoupled = 1;
  player->drive.shiftSpeedDelta = 0;
  player->drive.steeringGrip = 0;
  player->drive.trackCurveBias = 0;
  player->drive.trackCurveMode = 0;
  player->drive.jumpTimer = 0;
  player->drive.clutch = 0;
  player->drive.groundedFrames = 0;
  player->drive.launchEnergy = 0;
  player->drive.standingStartBounceY = 0;
  player->drive.standingStartBounceX = 0;
  player->drive.gear = 1;
  player->drive.engineRpm = 0;
  player->drive.reserved80 = 0;
  player->drive.drivetrainTorque = 0;
  player->drive.reserved7C = 0;
  player->drive.bodyLiftOffset = 0;
  player->drive.racePosition = 1;
  player->x = player->x + player->motionX;
  player->z = player->z + player->motionZ;
  player->facingBackwards = IsCarFacingBackwards(car);
  player->drive.jumpTimer = 0;
  player->drive.clutch = 0;
  player->drive.gearDisp = 1;
  player->drive.shiftRpmDelta = 0;
  g_ShiftTargetRpm = 0;
  drive = &car->drive;
  printf(g_MsgInit0);
  carSpec = g_CarSpec;
  if (carSpec->topGear < 6)
  {
    if (carSpec->topGear <= 0)
    {
      carSpec->topGear = 6;
    }
  }
  else
  {
    carSpec->topGear = 6;
  }
  drive->speedScale = (g_CarSpec->tachometer.speedScale * 0x490) / 160;
  printf(g_MsgInit1);
  j = 0;
  for (i = 0; i < 16; i++)
  {
    g_GearTorqueCurve[0].values[i] = g_CarSpec->torqueCurve[i] / 20;
    if (j < g_GearTorqueCurve[0].values[i])
    {
      g_PeakOutputRpm = i;
      j = g_GearTorqueCurve[0].values[i];
    }
  }

  g_PeakOutputValue = j;
  peakRpm = g_CarSpec->torqueBand.halves[g_PeakOutputRpm * 2];
  g_RedlineToPeakRpmHalf = (((s16) peakRpm) - g_CarSpec->redline) / 2;
  revLimitPtr = &g_CarSpec->revLimit;
  g_PeakToRevLimitRpmHalf = ((*revLimitPtr) - ((s16) peakRpm)) / 2;
  g_PeakOutputRpm = peakRpm;
  printf(g_MsgInit1b);
  printf(g_FmtDecimalLine, g_CarSpec->topGear);
  for (j = 0; j < 6; j++)
  {
    scaledGearRatio = (g_CarSpec->gearRatio[j + 1] * 0x490) / 160;
    g_CarSpec->gearLoad[j + 1] = (((scaledGearRatio * 6) / 100) << 17) / 10000;
    value = (g_CarSpec->torqueScale[j] * g_CarSpec->gearRatio[j + 1]) / 100;
    divisor = value;
    divisor = (divisor > 0) ? divisor : g_CarSpec->gearRatio[j + 1];
    for (i = 0; i < 16; i++)
    {
      g_GearTorqueCurve[j + 1].values[i] = g_CarSpec->torqueCurve[i] / divisor;
    }

  }

  if (g_CarSpec->baseSteeringGrip < 2)
  {
    g_CarSpec->baseSteeringGrip = 1;
  }
  printf(g_MsgInit2);
  curveSpec = g_CarSpec;
  accelBand = g_TorqueLossBandEnd;
  speedBandOffset = 0;
  speedThreshold = 0x3E8;
  torqueBand = g_TorqueBandEnd;
  do
  {
    for (i = 0; i < 16; i++)
    {
      if ((curveSpec->torqueBand.values[i] / speedThreshold) > 0)
      {
        *torqueBand = i;
        break;
      }
    }

    i = 0;
    bandSpeed = speedThreshold;
    accelBandOut = accelBand;
    while (i < 10)
    {
      if ((curveSpec->torqueLossRpm[i] / bandSpeed) > 0)
      {
        *accelBandOut = i;
        break;
      }
      i++;
    }

    accelBand++;
    speedBandOffset += 2;
    speedThreshold += 0x3E8;
    torqueBand++;
  }
  while (speedBandOffset < 20);
  printf(g_MsgInit4);
  drive->launchEnergyThreshold = g_LaunchEnergyThresholds[drive->launchThresholdIndex % 5] * 0xE;
  drive->steeringGripResponse = g_CarSpec->steeringGripResponse;
  printf(g_MsgInit5);
  player->shiftState = 0;
  drive->brakeLatch = 0;
  drive->acceleratorLatch = 0;
  g_EngineRpmJitter = 0;
  g_EngineRpm = 0;
  g_EngineRpmSnapshot = 0;
  g_StandingStartSpin = 0;
  g_DriveBoostTimer = 0;
  if (drive->manual != 0)
  {
    g_HudGlyphClut = 0x7800;
  }
  else
  {
    g_HudGlyphClut = 0x78CF;
  }
  g_DragScale = 0x3E8;
  g_SteerHoldFrames = 0;
  g_GripLossTimer = 0;
  g_WrongWayTimer = 0;
  g_PlayerAutoSteer = 0;
  printf(g_MsgInit6);
  printf(g_FmtLongLine, player->progressA);
  printf(g_MsgInitOk);
}

/*
 * Wrong-way / spin check: compares the car's headingAngle against the current
 * track point's forward direction (0xC00 - track angle) and returns whether the
 * delta falls inside the 0x401..0x7FF window (i.e. facing roughly backwards).
 */
s32 IsCarFacingBackwards(PlayerCarRuntime *car) {
    s32 index = car->trackPointIndex;
    s32 complement = 0xC00 - g_TrackPoints[index].angle;
    s32 diff = (car->headingAngle - complement) & 0xFFF;
    u32 backwardRange = diff - 0x401;
    return backwardRange < 0x7FFU;
}

/* The live button mapping; masks 0 and 1 steer, g_MirrorMode swaps them. */


/*
 * Steering-lean / body-roll state machine for the car `ctx`: drives the lean
 * steering lean and body roll from the drive-block steering input, branching
 * on the control mode g_RacePhase (0x41 = player, 0x23 = demo).
 */
void UpdateCarBodyRoll(PlayerCarRuntime *ctx) {
    GameCarDrive *p = &ctx->drive;
    s16 mode = g_RacePhase;
    s32 v1, a1;
    s32 a0v, r, s2v;

    if (mode < 2) {
        p->steerPos = 0;
        ctx->steeringAngle = 0;
    } else if ((mode < 4) && (g_PlayerAutoSteer == 0)) {
    if (g_PadType == 0x41) {

    if (g_MirrorMode != 0) {
        a1 = ReadStablePadHeld() & (s16)g_PadButtonMapping[0];
        v1 = ReadStablePadHeld() & (s16)g_PadButtonMapping[1];
    } else {
        v1 = ReadStablePadHeld() & (s16)g_PadButtonMapping[0];
        a1 = ReadStablePadHeld() & (s16)g_PadButtonMapping[1];
    }

    if (v1 != 0) {
    a0v = 2;
    if (ctx->facingBackwards != 0) a0v = 1;
    v1 = p->steerPos;
    p->trackCurveMode = a0v;
    if (v1 > 0) {
        p->steerPos = 0;
    } else if (v1 >= -4095) {
        p->steerPos = v1 - 1536;
    }
    ctx->bodyRollVelocity = ctx->bodyRollVelocity - 6;
    } else if (a1 != 0) {
    a0v = 1;
    if (ctx->facingBackwards != 0) a0v = 2;
    v1 = p->steerPos;
    p->trackCurveMode = a0v;
    if (v1 < 0) {
        p->steerPos = 0;
    } else if (v1 < 4096) {
        p->steerPos = v1 + 1536;
    }
    ctx->bodyRollVelocity = ctx->bodyRollVelocity + 6;
    } else {
    p->trackCurveMode = 0;
    p->steerPos = p->steerPos / 3;
    }
    ctx->steeringAngle = -p->steerPos;
    if (ctx->bodyRollVelocity != 0) {
        ctx->bodyRollVelocity = (ctx->bodyRollVelocity * 7) / 8;
    }
    } else if (g_PadType == 0x23) {
    a1 = ((g_NegconSteer * 13) << 9) / g_NegconSteerRange[g_NegconMaxTwist];
    if (g_MirrorMode != 0) a1 = -a1;
    if (!(a1 >= 0)) {

    a0v = 2;
    if (ctx->facingBackwards != 0) a0v = 1;
    v1 = p->steerPos;
    p->trackCurveMode = a0v;
    if (v1 > 0) {
        p->steerPos = 0;
        ctx->steeringAngle = 0;
    } else if ((a1 - 256) < v1) {
        s32 t;
        if (v1 >= 4097) v1 = 4096;
        t = rcos(v1 / 8);
        p->steerPos = p->steerPos - (t / 4);
        ctx->steeringAngle = ctx->steeringAngle + 1536;
    } else {
        p->steerPos = v1 / 3;
    }
    ctx->bodyRollVelocity = ctx->bodyRollVelocity - 6;
    } else if (!(a1 <= 0)) {
    a0v = 1;
    if (ctx->facingBackwards != 0) a0v = 2;
    v1 = p->steerPos;
    p->trackCurveMode = a0v;
    if (v1 < 0) {
        p->steerPos = 0;
        ctx->steeringAngle = 0;
    } else if (v1 < (a1 + 256)) {
        s32 t;
        s32 c = v1;
        if (v1 < -4096) { v1 = -4096; c = v1; }
        t = rcos(c / 8);
        p->steerPos = p->steerPos + (t / 4);
        ctx->steeringAngle = ctx->steeringAngle - 1536;
    } else {
        p->steerPos = v1 / 3;
    }
    ctx->bodyRollVelocity = ctx->bodyRollVelocity + 6;
    } else {
    p->trackCurveMode = 0;
    ctx->steeringAngle = ctx->steeringAngle / 2;
    p->steerPos = p->steerPos / 6;
    }
    if (ctx->bodyRollVelocity != 0) {
        ctx->bodyRollVelocity = (ctx->bodyRollVelocity * 7) / 8;
    }
    } else {
    ctx->bodyRollVelocity = 0;
    p->steerPos = 0;
    }
    } else {
    {
        s32 target = (ctx->facingBackwards << 11) + 3072;
        r = GetAngleDelta(ctx->bodyYaw, target - ctx->trackHeading.value);
    }
    s2v = r * 32;
    r = rcos(ctx->trackLateralOffset * 2);
    a0v = 4096 - r;
    if (ctx->speed < 800) {
        a0v = a0v * 6;
    } else {
        a0v = a0v * 4;
    }
    if (ctx->speed >= 81) {
        if (ctx->facingBackwards != 0) {
            if (ctx->trackLateralOffset < 0) a0v = -a0v;
        } else {
            if (ctx->trackLateralOffset > 0) a0v = -a0v;
        }
        a0v = a0v + s2v;
    } else {
        p->steerPos = 0;
        a0v = 0;
    }
    if (a0v < -4096) a0v = -4096;
    if (a0v > 4096) a0v = 4096;
    p->steerPos = a0v;
    ctx->steeringAngle = a0v;
    ctx->bodyRollVelocity = a0v / 128;

    }
    v1 = ctx->speed;
    if (v1 < 800) {
        s32 f = ctx->bodyRollVelocity;
        ctx->bodyRollVelocity = (f * v1) / 800;
    }
}

/*
 * Point-in-quad test: returns 1 if point `pt` is inside the quad with corners
 * p0,p1,p3,p2 (four chained half-plane sign checks via NormalClip), else 0.
 */
s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 pt) {
    s32 result;
    s32 ret = 0;

    if (NormalClip(p0, p1, pt) >= 0) {
        if (NormalClip(p1, p3, pt) >= 0) {
            if (NormalClip(p3, p2, pt) >= 0) {
                result = NormalClip(p2, p0, pt) >= 0;
                ret = result;
            }
        }
    }

    return ret;
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car)
{
  SVec rotation;
  s32 opponentX;
  Vec4 transformed;
  Matrix rotationMatrix;
  CarCollisionPoint velocityDelta;
  CarCollisionPoint playerOutline[6];
  CarCollisionPoint playerGrid[4][4];
  CarCollisionPoint opponentSamples[10];
  CarCollisionPoint opponentCorners[4];
  GameCarRuntime *opponent;
  s32 index;
  s32 sampleIndex;
  s32 quadIndex;
  s32 cornerIndex;
  s32 collisionRegion;
  s32 progressDelta;
  s32 playerProgress;
  s32 playerTrackOffset;
  s32 playerX;
  s32 trackDelta;
  if (g_GrandPrixMode == 0)
  {
    return 0;
  }
  opponent = g_Cars;
  collisionRegion = 0;
  rotation.vx = (u16)car->bodyPitch;
  rotation.vz = (u16)car->bodyRoll;
  rotation.vy = (u16)car->bodyYaw;
  RotMatrix(&rotation, &rotationMatrix);
  index = 0;
  do
  {
    rotation.vx = g_PlayerHullPoints[index].x;
    rotation.vz = g_PlayerHullPoints[index].z;
    rotation.vy = 0;
    ApplyMatrix(&rotationMatrix, &rotation, &transformed);
    playerOutline[index].x = transformed.x >> 1;
    playerOutline[index].z = transformed.z >> 1;
    if (index < 4)
    {
      playerGrid[index][index] = playerOutline[index];
    }
    index++;
  }
  while (index < 6);
  playerGrid[0][1].x = (playerGrid[1][0].x = (playerOutline[0].x + playerOutline[1].x) / 2);
  playerGrid[0][1].z = (playerGrid[1][0].z = (playerOutline[0].z + playerOutline[1].z) / 2);
  playerGrid[0][2].x = (playerGrid[2][0].x = playerOutline[4].x);
  playerGrid[0][2].z = (playerGrid[2][0].z = playerOutline[4].z);
  playerGrid[1][3].x = (playerGrid[3][1].x = playerOutline[5].x);
  playerGrid[1][3].z = (playerGrid[3][1].z = playerOutline[5].z);
  playerGrid[2][3].x = (playerGrid[3][2].x = (playerOutline[2].x + playerOutline[3].x) / 2);
  playerGrid[2][3].z = (playerGrid[3][2].z = (playerOutline[2].z + playerOutline[3].z) / 2);
  playerGrid[0][3].x = (playerGrid[1][2].x = (playerGrid[2][1].x = (playerGrid[3][0].x = (playerOutline[4].x + playerOutline[5].x) / 2)));
  playerGrid[0][3].z = (playerGrid[1][2].z = (playerGrid[2][1].z = (playerGrid[3][0].z = (playerOutline[4].z + playerOutline[5].z) / 2)));
  playerProgress = car->trackProgress;
  index = 0;
  playerTrackOffset = car->trackLateralOffset;
  playerX = car->y;
  for (; index < 11; index++, opponent++)
  {
    if (opponent->activeFlag != -1)
    {
      s32 aDist;
      opponent->collisionFlag = 0;
      progressDelta = (opponent->trackProgress + g_TrackLength - playerProgress) % g_TrackLength;
      opponentX = opponent->y;
      trackDelta = opponent->trackLateralOffset - playerTrackOffset;
      if (trackDelta < 0)
      {
        trackDelta = -trackDelta;
      }
      aDist = opponentX - playerX;
      if (aDist < 0)
      {
        aDist = -aDist;
      }
      if ((opponent->verticalMotionState == car->shiftState) || (aDist < 0x1A))
      {
        if (((trackDelta < 0x64) && ((progressDelta < 0xC8) || ((g_TrackLength - 0xC8) < progressDelta))) && (aDist < 0x3C))
        {
          if ((progressDelta < 0xC8) && (trackDelta < 0x32))
          {
            g_DragScale = 0x2BC;
          }
          velocityDelta.x = (u16)opponent->x - (u16)car->x;
          velocityDelta.z = (u16)opponent->z - (u16)car->z;
          rotation.vx = (u16)opponent->bodyPitch;
          rotation.vz = (u16)opponent->bodyRoll;
          rotation.vy = (u16)opponent->bodyYaw;
          RotMatrix(&rotation, &rotationMatrix);
          for (cornerIndex = 0; cornerIndex < 4; cornerIndex++)
          {
            rotation.vx = g_OpponentHullCorners[cornerIndex].x;
            rotation.vz = g_OpponentHullCorners[cornerIndex].z;
            rotation.vy = 0;
            ApplyMatrix(&rotationMatrix, &rotation, &transformed);
            opponentCorners[cornerIndex].x =
                (transformed.x >> 1) + (velocityDelta.x / 2);
            opponentCorners[cornerIndex].z =
                (transformed.z >> 1) + (velocityDelta.z / 2);
          }

          opponentSamples[0].x = (opponentCorners[0].x + opponentCorners[1].x) / 2;
          opponentSamples[0].z = (opponentCorners[0].z + opponentCorners[1].z) / 2;
          opponentSamples[1].x = (opponentCorners[0].x + opponentCorners[2].x) / 2;
          opponentSamples[1].z = (opponentCorners[0].z + opponentCorners[2].z) / 2;
          opponentSamples[2].x = (opponentCorners[1].x + opponentCorners[3].x) / 2;
          opponentSamples[2].z = (opponentCorners[1].z + opponentCorners[3].z) / 2;
          opponentSamples[3].x = (opponentCorners[2].x + opponentCorners[3].x) / 2;
          opponentSamples[3].z = (opponentCorners[2].z + opponentCorners[3].z) / 2;
          opponentSamples[4].x = (opponentSamples[0].x + opponentSamples[2].x) / 2;
          opponentSamples[4].z = (opponentSamples[0].z + opponentSamples[2].z) / 2;
          opponentSamples[6].x = (opponentCorners[0].x + opponentSamples[1].x) / 2;
          opponentSamples[6].z = (opponentCorners[0].z + opponentSamples[1].z) / 2;
          opponentSamples[7].x = (opponentCorners[1].x + opponentSamples[2].x) / 2;
          opponentSamples[7].z = (opponentCorners[1].z + opponentSamples[2].z) / 2;
          opponentSamples[8].x = (opponentCorners[2].x + opponentSamples[1].x) / 2;
          opponentSamples[8].z = (opponentCorners[2].z + opponentSamples[1].z) / 2;
          opponentSamples[9].x = (opponentCorners[3].x + opponentSamples[2].x) / 2;
          opponentSamples[9].z = (opponentCorners[3].z + opponentSamples[2].z) / 2;
          for (sampleIndex = 0; sampleIndex < 4; sampleIndex++)
          {
            for (quadIndex = 0; quadIndex < 4; quadIndex++)
            {
              collisionRegion = IsPointInQuad(
                GetCarCollisionPointPacked(&playerGrid[quadIndex][2]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][3]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][0]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][1]),
                GetCarCollisionPointPacked(&opponentCorners[sampleIndex]));
              if (collisionRegion > 0)
              {
                collisionRegion = quadIndex + 1;
                break;
              }
            }

            if (collisionRegion > 0)
            {
              goto collision_found;
            }
          }

          for (sampleIndex = 0; sampleIndex < 5; sampleIndex++)
          {
            for (quadIndex = 0; quadIndex < 4; quadIndex++)
            {
              collisionRegion = IsPointInQuad(
                GetCarCollisionPointPacked(&playerGrid[quadIndex][2]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][3]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][0]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][1]),
                GetCarCollisionPointPacked(&opponentSamples[sampleIndex]));
              if (collisionRegion > 0)
              {
                collisionRegion = quadIndex + 1;
                break;
              }
            }

            if (collisionRegion > 0)
            {
              goto collision_found;
            }
          }

          for (sampleIndex = 0; sampleIndex < 4; sampleIndex++)
          {
            for (quadIndex = 0; quadIndex < 4; quadIndex++)
            {
              collisionRegion = IsPointInQuad(
                GetCarCollisionPointPacked(&playerGrid[quadIndex][2]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][3]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][0]),
                GetCarCollisionPointPacked(&playerGrid[quadIndex][1]),
                GetCarCollisionPointPacked(&opponentSamples[sampleIndex + 6]));
              if (collisionRegion > 0)
              {
                collisionRegion = quadIndex + 1;
                break;
              }
            }

            if (collisionRegion > 0)
            {
              goto collision_found;
            }
          }

        }
        else
          if ((trackDelta < 0x32) && (progressDelta < 0x3E8))
        {
          s32 v = 0x3E8 - progressDelta;
          if (v < 0)
          {
            v += 3;
          }
          g_DragScale = 0x3E8 - (v >> 2);
        }
      }
    }
  }

  if (collisionRegion <= 0)
  {
    return collisionRegion;
  }
  collision_found:
  if (((s16)car->motionTimer < 0xB) && (g_RacePhase < 3))
  {
    s32 sid;
    u32 collisionAngleRange;
    collisionAngleRange = trackDelta + 0x1D;
    if (collisionAngleRange < 0x3B)
    {
      sid = 0xA;
      if (collisionRegion >= 3)
      {
        sid = 0xD;
      }
    }
    else
    {
      sid = 0xC;
      if ((collisionRegion & 1) != g_MirrorMode)
      {
        sid = 0xB;
      }
    }
    PlaySoundCue(sid);
  }

  trackDelta = car->trackLateralOffset - opponent->trackLateralOffset;
  g_GripLossTimer = 0;
  if (collisionRegion < 3)
  {
    if (car->facingBackwards != ReadStableRaceSeries())
    {
      car->drive.drivetrainTorque = 0;
      car->acceleration = 0;
    }
    else
    {
      car->acceleration = car->acceleration / 2;
      car->drive.drivetrainTorque = car->drive.drivetrainTorque * 0x50 / 100;
    }
    if ((car->speed - opponent->speed) >= 0x191)
    {
      g_GripLossTimer = 0x1E;
    }
    else
    {
      g_GripLossTimer = 0xF;
    }
    {
      s32 vx;
      s32 vz;
      vx = (s16)((u16)opponent->worldVelocityX - (u16)car->drive.accelPos);
      velocityDelta.x = vx / 0x20;
      vz = (s16)((u16)opponent->worldVelocityZ - (u16)car->drive.brakePos);
      velocityDelta.z = vz / 0x20;
    }
    if (((car->facingBackwards != ReadStableRaceSeries()) && (car->speed >= 0x51)) && (g_WrongWayTimer >= 0xA))
    {
      SetCarKnockback(opponent, 0, 0, 4);
      SetCarKnockback(car, 0, 0, 4);
    }
    else
    {
      if (car->speed >= 0x29)
      {
        SetCarKnockback(car, 0, 0, 4);
      }
      else
      {
        SetCarKnockback(car, -((s16) velocityDelta.x), -((s16) velocityDelta.z), 4);
      }
      SetCarKnockback(opponent, (s16) velocityDelta.x, (s16) velocityDelta.z, 4);
    }
  }
  else
  {
    s32 vx;
    s32 vz;
    opponent->speed = opponent->speed / 2;
    opponent->acceleration = opponent->acceleration / 2;
    opponent->boostTimer = opponent->collisionBoostDuration;
    vx = (s16)((u16)opponent->worldVelocityX - (u16)car->drive.accelPos);
    velocityDelta.x = vx / 0x20;
    vz = (s16)((u16)opponent->worldVelocityZ - (u16)car->drive.brakePos);
    velocityDelta.z = vz / 0x20;
    velocityDelta.x = velocityDelta.x - (u16)opponent->velocityX;
    velocityDelta.z = velocityDelta.z - (u16)opponent->velocityZ;
    if (((car->facingBackwards != ReadStableRaceSeries()) && (car->speed >= 0x51)) && (g_WrongWayTimer >= 0xA))
    {
      SetCarKnockback(opponent, 0, 0, 4);
      SetCarKnockback(car, 0, 0, 4);
    }
    else
    {
      SetCarKnockback(car, -((s16) velocityDelta.x), -((s16) velocityDelta.z), 4);
      SetCarKnockback(opponent, 0, 0, 4);
    }
  }
  opponent->collisionFlag = 1;
  return collisionRegion;
}
