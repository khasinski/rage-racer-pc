#include "game/diagnostics.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/audio.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"

#include <stdlib.h>

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
