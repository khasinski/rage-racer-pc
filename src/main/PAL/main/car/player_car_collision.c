#include "game/diagnostics.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/audio.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"

#include <stdlib.h>

static CarCollisionPoint Midpoint(CarCollisionPoint a, CarCollisionPoint b) {
  CarCollisionPoint result;

  result.x = (a.x + b.x) / 2;
  result.z = (a.z + b.z) / 2;
  return result;
}

static void BuildPlayerCollisionGrid(const PlayerCarRuntime *car,
                                     CarCollisionPoint grid[4][4]) {
  CarCollisionPoint outline[6];
  Matrix rotationMatrix;
  SVec input;
  Vec4 transformed;
  s32 index;

  input.vx = (u16)car->bodyPitch;
  input.vz = (u16)car->bodyRoll;
  input.vy = (u16)car->bodyYaw;
  RotMatrix(&input, &rotationMatrix);
  for (index = 0; index < 6; index++) {
    input.vx = g_PlayerHullPoints[index].x;
    input.vy = 0;
    input.vz = g_PlayerHullPoints[index].z;
    ApplyMatrix(&rotationMatrix, &input, &transformed);
    outline[index].x = transformed.x >> 1;
    outline[index].z = transformed.z >> 1;
    if (index < 4) {
      grid[index][index] = outline[index];
    }
  }

  grid[0][1] = grid[1][0] = Midpoint(outline[0], outline[1]);
  grid[0][2] = grid[2][0] = outline[4];
  grid[1][3] = grid[3][1] = outline[5];
  grid[2][3] = grid[3][2] = Midpoint(outline[2], outline[3]);
  grid[0][3] = grid[1][2] = grid[2][1] = grid[3][0] =
      Midpoint(outline[4], outline[5]);
}

static void BuildOpponentCollisionSamples(const PlayerCarRuntime *player,
                                          const GameCarRuntime *opponent,
                                          CarCollisionPoint corners[4],
                                          CarCollisionPoint samples[10]) {
  Matrix rotationMatrix;
  SVec input;
  Vec4 transformed;
  s32 offsetX = (u16)opponent->x - (u16)player->x;
  s32 offsetZ = (u16)opponent->z - (u16)player->z;
  s32 index;

  input.vx = (u16)opponent->bodyPitch;
  input.vz = (u16)opponent->bodyRoll;
  input.vy = (u16)opponent->bodyYaw;
  RotMatrix(&input, &rotationMatrix);
  for (index = 0; index < 4; index++) {
    input.vx = g_OpponentHullCorners[index].x;
    input.vy = 0;
    input.vz = g_OpponentHullCorners[index].z;
    ApplyMatrix(&rotationMatrix, &input, &transformed);
    corners[index].x = (transformed.x >> 1) + offsetX / 2;
    corners[index].z = (transformed.z >> 1) + offsetZ / 2;
  }

  samples[0] = Midpoint(corners[0], corners[1]);
  samples[1] = Midpoint(corners[0], corners[2]);
  samples[2] = Midpoint(corners[1], corners[3]);
  samples[3] = Midpoint(corners[2], corners[3]);
  samples[4] = Midpoint(samples[0], samples[2]);
  samples[6] = Midpoint(corners[0], samples[1]);
  samples[7] = Midpoint(corners[1], samples[2]);
  samples[8] = Midpoint(corners[2], samples[1]);
  samples[9] = Midpoint(corners[3], samples[2]);
}

static s32 FindPlayerCollisionRegion(CarCollisionPoint grid[4][4],
                                     const CarCollisionPoint corners[4],
                                     const CarCollisionPoint samples[10],
                                     s32 *sampleIndex, s32 *quadIndex) {
  s32 region = FirstQuadHit(grid, corners, 4, sampleIndex, quadIndex);

  if (region <= 0) {
    region = FirstQuadHit(grid, samples, 5, sampleIndex, quadIndex);
  }
  if (region <= 0) {
    region = FirstQuadHit(grid, &samples[6], 4, sampleIndex, quadIndex);
  }
  return region;
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car)
{
  s32 opponentX;
  CarCollisionPoint velocityDelta;
  CarCollisionPoint playerGrid[4][4];
  CarCollisionPoint opponentSamples[10];
  CarCollisionPoint opponentCorners[4];
  GameCarRuntime *opponent;
  s32 index;
  s32 sampleIndex;
  s32 quadIndex;
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
  BuildPlayerCollisionGrid(car, playerGrid);
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
          BuildOpponentCollisionSamples(car, opponent, opponentCorners,
                                        opponentSamples);
          collisionRegion = FindPlayerCollisionRegion(
              playerGrid, opponentCorners, opponentSamples,
              &sampleIndex, &quadIndex);
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
