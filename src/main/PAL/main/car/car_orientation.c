#include "game/diagnostics.h"
#include <stdio.h>
#include "game/track.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/audio.h"
#include <stdlib.h>

void InitPlayerCar(PlayerCarRuntime *car)
{
  CarTrackLimits trackState = {0};
  Matrix rotationMatrix;
  Matrix axisMatrix;
  SVec rotationOffset;
  PlayerCarRuntime *player;
  GameCarDrive *drive;
  TrackEventData *eventData;
  s32 headingBase;
  s32 value;
  player = car;
  eventData = g_TrackEventData;
  printf("%s", g_MsgInitCar);
  value = g_GrandPrixSeries;
  g_RacePhase = 2;
  g_RaceSeries = value & 1;
  BuildTachoNeedleQuad();
  ClearCarMotionState(AsRivalCar(car));
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
  printf("%s", g_MsgHTbl);
  player->trackPointIndex = eventData->rivalStarts[ReadStableRaceSeries()][0].trackPointIndex;
  player->x = eventData->rivalStarts[ReadStableRaceSeries()][0].x;
  player->z = eventData->rivalStarts[ReadStableRaceSeries()][0].z;
  player->y = 0;
  player->trackPointIndex = FindTrackSegment(AsRivalCar(car), player->trackPointIndex);
  player->bodyPitch = 0;
  headingBase = 0xC00 - (ReadStableRaceSeries() << 11);
  player->bodyYaw = (headingBase - TrackPoint(player->trackPointIndex)->angle) & 0xFFF;
  player->bodyRoll = 0;
  player->bodyRollVelocity = 0;
  player->previousTrackPointIndex = player->trackPointIndex;
  player->headingAngle = player->bodyYaw;
  player->drive.targetHeading = player->headingAngle;
  SeedCarLapProgress(GetPlayerCarRuntime(car), 0);
  trackState.rightInset = 0;
  trackState.leftInset = 0;
  UpdateCarTrackState(AsRivalCar(car), player->trackPointIndex, &trackState);
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
  player->drive.racePosition = 1;
  player->x = player->x + player->motionX;
  player->z = player->z + player->motionZ;
  player->facingBackwards = IsCarFacingBackwards(car);
  player->drive.gearDisp = 1;
  player->drive.shiftRpmDelta = 0;
  g_ShiftTargetRpm = 0;
  drive = &car->drive;
  printf("%s", g_MsgInit0);
  printf("%s", g_MsgInit1);
  PrepareCarPerformance(drive);
  printf("%s", g_MsgInit5);
  player->verticalMotionState = 0;
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
  printf("%s", g_MsgInit6);
  printf(g_FmtLongLine, player->progressA);
  printf("%s", g_MsgInitOk);
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
      if ((opponent->verticalMotionState == car->verticalMotionState) || (aDist < 0x1A))
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
          /* Corners first, then the edge and centre points, then the
             points between: the cheapest test that can hit, first. */
          collisionRegion = FirstQuadHit(playerGrid, opponentCorners, 4,
                                         &sampleIndex, &quadIndex);
          if (collisionRegion <= 0)
          {
            collisionRegion = FirstQuadHit(playerGrid, opponentSamples, 5,
                                           &sampleIndex, &quadIndex);
          }
          if (collisionRegion <= 0)
          {
            collisionRegion = FirstQuadHit(playerGrid, &opponentSamples[6], 4,
                                           &sampleIndex, &quadIndex);
          }
          if (collisionRegion > 0)
          {
            /* The check after the loop sends a hit to the same place. */
            break;
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
  if (DiagnosticsEnabled("car.collision_trace"))
  {
    const char *timerText = DiagnosticsValue("car.collision_trace_timer");
    if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0))
    {
      Trace("car-collision", "timer=%d opponent=%d region=%d sample=%d quad=%d "
             "player=%d,%d,%d,%d opponent_state=%d,%d,%d,%d delta=%d,%d",
             g_SceneTimer, index, collisionRegion, sampleIndex, quadIndex,
             car->x, car->z, car->trackProgress, car->trackLateralOffset,
             opponent->x, opponent->z, opponent->trackProgress,
             opponent->trackLateralOffset, progressDelta, trackDelta);
    }
  }
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
      SetCarKnockback(AsRivalCar(car), 0, 0, 4);
    }
    else
    {
      if (car->speed >= 0x29)
      {
        SetCarKnockback(AsRivalCar(car), 0, 0, 4);
      }
      else
      {
        SetCarKnockback(AsRivalCar(car), -((s16) velocityDelta.x),
                        -((s16) velocityDelta.z), 4);
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
      SetCarKnockback(AsRivalCar(car), 0, 0, 4);
    }
    else
    {
      SetCarKnockback(AsRivalCar(car), -((s16) velocityDelta.x),
                        -((s16) velocityDelta.z), 4);
      SetCarKnockback(opponent, 0, 0, 4);
    }
  }
  opponent->collisionFlag = 1;
  return collisionRegion;
}
