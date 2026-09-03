#include "game/diagnostics.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/audio.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"

enum {
    COLLISION_PROGRESS_REACH = 0xC8,
    COLLISION_LATERAL_REACH = 0x64,
    COLLISION_HEIGHT_REACH = 0x3C,
    DIFFERENT_LEVEL_HEIGHT = 0x1A,
    SLIPSTREAM_LATERAL_REACH = 0x32,
    SLIPSTREAM_PROGRESS_REACH = 0x3E8,
    CLOSE_SLIPSTREAM_DRAG = 0x2BC,
    PLAYER_HULL_POINT_COUNT = 6,
    OPPONENT_COLLISION_SAMPLE_COUNT = 9,
    COARSE_COLLISION_SAMPLE_COUNT = 5,
};

static CarCollisionPoint Midpoint(CarCollisionPoint a, CarCollisionPoint b) {
    CarCollisionPoint result;

    result.x = (a.x + b.x) / 2;
    result.z = (a.z + b.z) / 2;
    return result;
}

static void BuildPlayerCollisionGrid(const PlayerCarRuntime *car,
                                     CarCollisionPoint
                                         grid[CAR_COLLISION_QUAD_COUNT]
                                             [CAR_COLLISION_QUAD_COUNT]) {
    CarCollisionPoint outline[PLAYER_HULL_POINT_COUNT];
    Matrix rotationMatrix;
    SVec input;
    Vec4 transformed;
    s32 index;

    input.vx = (u16)car->bodyPitch;
    input.vz = (u16)car->bodyRoll;
    input.vy = (u16)car->bodyYaw;
    RotMatrix(&input, &rotationMatrix);
    for (index = 0; index < PLAYER_HULL_POINT_COUNT; index++) {
        input.vx = g_PlayerHullPoints[index].x;
        input.vy = 0;
        input.vz = g_PlayerHullPoints[index].z;
        ApplyMatrix(&rotationMatrix, &input, &transformed);
        outline[index].x = transformed.x >> 1;
        outline[index].z = transformed.z >> 1;
        if (index < CAR_COLLISION_QUAD_COUNT) {
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
                                          CarCollisionPoint
                                              corners[CAR_COLLISION_QUAD_COUNT],
                                          CarCollisionPoint samples
                                              [OPPONENT_COLLISION_SAMPLE_COUNT]) {
    Matrix rotationMatrix;
    SVec input;
    Vec4 transformed;
    s32 offsetX = (s16)((u16)opponent->x - (u16)player->x);
    s32 offsetZ = (s16)((u16)opponent->z - (u16)player->z);
    s32 index;

    input.vx = (u16)opponent->bodyPitch;
    input.vz = (u16)opponent->bodyRoll;
    input.vy = (u16)opponent->bodyYaw;
    RotMatrix(&input, &rotationMatrix);
    for (index = 0; index < CAR_COLLISION_QUAD_COUNT; index++) {
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
    samples[5] = Midpoint(corners[0], samples[1]);
    samples[6] = Midpoint(corners[1], samples[2]);
    samples[7] = Midpoint(corners[2], samples[1]);
    samples[8] = Midpoint(corners[3], samples[2]);
}

static CarCollisionHit FindPlayerCollisionRegion(
    const CarCollisionPoint
        grid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT],
    const CarCollisionPoint corners[CAR_COLLISION_QUAD_COUNT],
    const CarCollisionPoint samples[OPPONENT_COLLISION_SAMPLE_COUNT]) {
    CarCollisionHit hit = FindFirstCarCollisionQuad(
        grid, corners, CAR_COLLISION_QUAD_COUNT);

    if (hit.region <= 0) {
        hit = FindFirstCarCollisionQuad(
            grid, samples, COARSE_COLLISION_SAMPLE_COUNT);
    }
    if (hit.region <= 0) {
        hit = FindFirstCarCollisionQuad(
            grid, &samples[COARSE_COLLISION_SAMPLE_COUNT],
            OPPONENT_COLLISION_SAMPLE_COUNT -
                COARSE_COLLISION_SAMPLE_COUNT);
    }
    return hit;
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
    int64_t difference = (int64_t)a - b;

    if (difference < 0) {
        difference = -difference;
    }
    return difference > INT32_MAX ? INT32_MAX : (s32)difference;
}

static PlayerCollisionHit FindPlayerCollision(
    PlayerCarRuntime *player,
    CarCollisionPoint
        playerGrid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT]) {
    PlayerCollisionHit hit = {0};
    CarCollisionHit quadHit;
    CarCollisionPoint samples[OPPONENT_COLLISION_SAMPLE_COUNT];
    CarCollisionPoint corners[CAR_COLLISION_QUAD_COUNT];
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *opponent = &g_Cars[index];
        s32 heightDistance;
        s32 progressDistance;
        s32 lateralDistance;

        if (opponent->activeFlag == -1) {
            continue;
        }
        progressDistance = (opponent->trackProgress + g_TrackLength -
                            player->trackProgress) % g_TrackLength;
        lateralDistance = AbsoluteDifference(opponent->trackLateralOffset,
                                             player->trackLateralOffset);
        heightDistance = AbsoluteDifference(opponent->y, player->y);
        if (opponent->verticalMotionState != player->verticalMotionState &&
            heightDistance >= DIFFERENT_LEVEL_HEIGHT) {
            continue;
        }

        if (lateralDistance < COLLISION_LATERAL_REACH &&
            (progressDistance < COLLISION_PROGRESS_REACH ||
             g_TrackLength - COLLISION_PROGRESS_REACH < progressDistance) &&
            heightDistance < COLLISION_HEIGHT_REACH) {
            if (progressDistance < COLLISION_PROGRESS_REACH &&
                lateralDistance < SLIPSTREAM_LATERAL_REACH) {
                g_DragScale = CLOSE_SLIPSTREAM_DRAG;
            }
            BuildOpponentCollisionSamples(player, opponent, corners, samples);
            quadHit = FindPlayerCollisionRegion(playerGrid, corners, samples);
            hit.region = quadHit.region;
            hit.sampleIndex = quadHit.sampleIndex;
            hit.quadIndex = quadHit.quadIndex;
            if (hit.region > 0) {
                hit.opponent = opponent;
                hit.opponentIndex = index;
                hit.progressDistance = progressDistance;
                hit.lateralDistance = lateralDistance;
                return hit;
            }
        } else if (lateralDistance < SLIPSTREAM_LATERAL_REACH &&
                   progressDistance < SLIPSTREAM_PROGRESS_REACH) {
            g_DragScale = SLIPSTREAM_PROGRESS_REACH -
                ((SLIPSTREAM_PROGRESS_REACH - progressDistance) >> 2);
        }
    }
    return hit;
}

static void TracePlayerCollision(const PlayerCarRuntime *player,
                                 const PlayerCollisionHit *hit) {
    if (!DiagnosticsEnabled("car.collision_trace")) {
        return;
    }
    if (g_SceneTimer != DiagnosticsIntValue(
            "car.collision_trace_timer", g_SceneTimer)) {
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
    if (hit->lateralDistance < 30) {
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
        velocity.x = (s16)(velocity.x - (s16)opponent->velocityX);
        velocity.z = (s16)(velocity.z - (s16)opponent->velocityZ);
    }
    return velocity;
}

static s32 IsWrongWayImpact(const PlayerCarRuntime *player) {
    return player->facingBackwards != g_RaceSeries &&
           player->speed >= 0x51 && g_WrongWayTimer >= 0xA;
}

static void ApplyLowRegionCollision(PlayerCarRuntime *player,
                                    GameCarRuntime *opponent) {
    CarCollisionPoint velocity = GetCollisionVelocity(player, opponent, 0);

    if (player->facingBackwards != g_RaceSeries) {
        player->drive.drivetrainTorque = 0;
        player->acceleration = 0;
    } else {
        player->acceleration /= 2;
        player->drive.drivetrainTorque =
            player->drive.drivetrainTorque * 0x50 / 100;
    }
    g_GripLossTimer =
        player->speed - opponent->speed >= 0x191 ? 0x1E : 0xF;

    if (IsWrongWayImpact(player)) {
        SetCarKnockback(opponent, 0, 0, CAR_KNOCKBACK_VECTOR_MODE);
        SetCarKnockback(AsRivalCar(player), 0, 0,
                        CAR_KNOCKBACK_VECTOR_MODE);
        return;
    }
    if (player->speed >= 0x29) {
        SetCarKnockback(AsRivalCar(player), 0, 0,
                        CAR_KNOCKBACK_VECTOR_MODE);
    } else {
        SetCarKnockback(AsRivalCar(player), -velocity.x, -velocity.z,
                        CAR_KNOCKBACK_VECTOR_MODE);
    }
    SetCarKnockback(opponent, velocity.x, velocity.z,
                    CAR_KNOCKBACK_VECTOR_MODE);
}

static void ApplyHighRegionCollision(PlayerCarRuntime *player,
                                     GameCarRuntime *opponent) {
    CarCollisionPoint velocity;

    opponent->speed /= 2;
    opponent->acceleration /= 2;
    opponent->boostTimer = opponent->collisionBoostDuration;
    velocity = GetCollisionVelocity(player, opponent, 1);
    if (IsWrongWayImpact(player)) {
        SetCarKnockback(opponent, 0, 0, CAR_KNOCKBACK_VECTOR_MODE);
        SetCarKnockback(AsRivalCar(player), 0, 0,
                        CAR_KNOCKBACK_VECTOR_MODE);
    } else {
        SetCarKnockback(AsRivalCar(player), -velocity.x, -velocity.z,
                        CAR_KNOCKBACK_VECTOR_MODE);
        SetCarKnockback(opponent, 0, 0, CAR_KNOCKBACK_VECTOR_MODE);
    }
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car) {
    CarCollisionPoint playerGrid[CAR_COLLISION_QUAD_COUNT]
                                [CAR_COLLISION_QUAD_COUNT];
    PlayerCollisionHit hit;
    s32 index;

    if (g_GrandPrixMode == 0 || g_TrackLength <= 0) {
        return 0;
    }

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].collisionFlag = 0;
    }

    BuildPlayerCollisionGrid(car, playerGrid);
    hit = FindPlayerCollision(car, playerGrid);
    if (hit.region <= 0) {
        return hit.region;
    }

    TracePlayerCollision(car, &hit);
    PlayPlayerCollisionSound(car, &hit);
    g_GripLossTimer = 0;
    if (hit.region <= LAST_FRONT_COLLISION_REGION) {
        ApplyLowRegionCollision(car, hit.opponent);
    } else {
        ApplyHighRegionCollision(car, hit.opponent);
    }
    hit.opponent->collisionFlag = 1;
    return hit.region;
}
