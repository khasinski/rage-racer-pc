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

typedef struct PlayerCollisionHit {
  GameCarRuntime *opponent;
  s32 opponentIndex;
  s32 region;
  s32 sampleIndex;
  s32 quadIndex;
  s32 progressDistance;
  s32 lateralDistance;
} PlayerCollisionHit;

static s32 AbsoluteDifference(s32 a, s32 b) {
  s32 difference = a - b;
  return difference < 0 ? -difference : difference;
}

static PlayerCollisionHit FindPlayerCollision(
    PlayerCarRuntime *player, CarCollisionPoint playerGrid[4][4]) {
  PlayerCollisionHit hit = {0};
  CarCollisionPoint samples[10];
  CarCollisionPoint corners[4];
  s32 index;

  for (index = 0; index < 11; index++) {
    GameCarRuntime *opponent = &g_Cars[index];
    s32 heightDistance;
    s32 progressDistance;
    s32 lateralDistance;

    if (opponent->activeFlag == -1) {
      continue;
    }
    opponent->collisionFlag = 0;
    progressDistance = (opponent->trackProgress + g_TrackLength -
                        player->trackProgress) % g_TrackLength;
    lateralDistance = AbsoluteDifference(opponent->trackLateralOffset,
                                         player->trackLateralOffset);
    heightDistance = AbsoluteDifference(opponent->y, player->y);
    if (opponent->verticalMotionState != player->verticalMotionState &&
        heightDistance >= 0x1A) {
      continue;
    }

    if (lateralDistance < 0x64 &&
        (progressDistance < 0xC8 ||
         g_TrackLength - 0xC8 < progressDistance) &&
        heightDistance < 0x3C) {
      if (progressDistance < 0xC8 && lateralDistance < 0x32) {
        g_DragScale = 0x2BC;
      }
      BuildOpponentCollisionSamples(player, opponent, corners, samples);
      hit.region = FindPlayerCollisionRegion(
          playerGrid, corners, samples, &hit.sampleIndex, &hit.quadIndex);
      if (hit.region > 0) {
        hit.opponent = opponent;
        hit.opponentIndex = index;
        hit.progressDistance = progressDistance;
        hit.lateralDistance = lateralDistance;
        return hit;
      }
    } else if (lateralDistance < 0x32 && progressDistance < 0x3E8) {
      g_DragScale = 0x3E8 - ((0x3E8 - progressDistance) >> 2);
    }
  }
  return hit;
}

static void TracePlayerCollision(const PlayerCarRuntime *player,
                                 const PlayerCollisionHit *hit) {
  const char *timerText;

  if (!DiagnosticsEnabled("car.collision_trace")) {
    return;
  }
  timerText = DiagnosticsValue("car.collision_trace_timer");
  if (timerText != NULL &&
      g_SceneTimer != (s32)strtol(timerText, NULL, 0)) {
    return;
  }
  Trace("car-collision", "timer=%d opponent=%d region=%d sample=%d quad=%d "
        "player=%d,%d,%d,%d opponent_state=%d,%d,%d,%d delta=%d,%d",
        g_SceneTimer, hit->opponentIndex, hit->region, hit->sampleIndex,
        hit->quadIndex, player->x, player->z, player->trackProgress,
        player->trackLateralOffset, hit->opponent->x, hit->opponent->z,
        hit->opponent->trackProgress, hit->opponent->trackLateralOffset,
        hit->progressDistance, hit->lateralDistance);
}

static void PlayPlayerCollisionSound(const PlayerCarRuntime *player,
                                     const PlayerCollisionHit *hit) {
  s32 soundCue;

  if ((s16)player->motionTimer >= 0xB || g_RacePhase >= 3) {
    return;
  }
  if ((u32)(hit->lateralDistance + 0x1D) < 0x3B) {
    soundCue = hit->region >= 3 ? 0xD : 0xA;
  } else {
    soundCue = (hit->region & 1) != g_MirrorMode ? 0xB : 0xC;
  }
  PlaySoundCue(soundCue);
}

static CarCollisionPoint GetCollisionVelocity(
    const PlayerCarRuntime *player, const GameCarRuntime *opponent,
    s32 includeOpponentMotion) {
  CarCollisionPoint velocity;
  s32 x = (s16)((u16)opponent->worldVelocityX -
                (u16)player->drive.accelPos);
  s32 z = (s16)((u16)opponent->worldVelocityZ -
                (u16)player->drive.brakePos);

  velocity.x = x / 0x20;
  velocity.z = z / 0x20;
  if (includeOpponentMotion) {
    velocity.x -= (u16)opponent->velocityX;
    velocity.z -= (u16)opponent->velocityZ;
  }
  return velocity;
}

static s32 IsWrongWayImpact(const PlayerCarRuntime *player) {
  return player->facingBackwards != ReadStableRaceSeries() &&
         player->speed >= 0x51 && g_WrongWayTimer >= 0xA;
}

static void ApplyLowRegionCollision(PlayerCarRuntime *player,
                                    GameCarRuntime *opponent) {
  CarCollisionPoint velocity = GetCollisionVelocity(player, opponent, 0);

  if (player->facingBackwards != ReadStableRaceSeries()) {
    player->drive.drivetrainTorque = 0;
    player->acceleration = 0;
  } else {
    player->acceleration /= 2;
    player->drive.drivetrainTorque =
        player->drive.drivetrainTorque * 0x50 / 100;
  }
  g_GripLossTimer = player->speed - opponent->speed >= 0x191 ? 0x1E : 0xF;

  if (IsWrongWayImpact(player)) {
    SetCarKnockback(opponent, 0, 0, 4);
    SetCarKnockback(AsRivalCar(player), 0, 0, 4);
    return;
  }
  if (player->speed >= 0x29) {
    SetCarKnockback(AsRivalCar(player), 0, 0, 4);
  } else {
    SetCarKnockback(AsRivalCar(player), -(s16)velocity.x,
                    -(s16)velocity.z, 4);
  }
  SetCarKnockback(opponent, (s16)velocity.x, (s16)velocity.z, 4);
}

static void ApplyHighRegionCollision(PlayerCarRuntime *player,
                                     GameCarRuntime *opponent) {
  CarCollisionPoint velocity;

  opponent->speed /= 2;
  opponent->acceleration /= 2;
  opponent->boostTimer = opponent->collisionBoostDuration;
  velocity = GetCollisionVelocity(player, opponent, 1);
  if (IsWrongWayImpact(player)) {
    SetCarKnockback(opponent, 0, 0, 4);
    SetCarKnockback(AsRivalCar(player), 0, 0, 4);
  } else {
    SetCarKnockback(AsRivalCar(player), -(s16)velocity.x,
                    -(s16)velocity.z, 4);
    SetCarKnockback(opponent, 0, 0, 4);
  }
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car) {
  CarCollisionPoint playerGrid[4][4];
  PlayerCollisionHit hit;

  if (g_GrandPrixMode == 0) {
    return 0;
  }

  BuildPlayerCollisionGrid(car, playerGrid);
  hit = FindPlayerCollision(car, playerGrid);
  if (hit.region <= 0) {
    return hit.region;
  }

  TracePlayerCollision(car, &hit);
  PlayPlayerCollisionSound(car, &hit);
  g_GripLossTimer = 0;
  if (hit.region < 3) {
    ApplyLowRegionCollision(car, hit.opponent);
  } else {
    ApplyHighRegionCollision(car, hit.opponent);
  }
  hit.opponent->collisionFlag = 1;
  return hit.region;
}
