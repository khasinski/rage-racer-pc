#include "game/car_collision_internal.h"
#include "game/integer.h"
#include "game/vector.h"
#include "game/hull_rotation.h"

#include <stddef.h>

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

static void BuildPlayerCollisionGrid(const PlayerCarRuntime *car,
                                     const CarHullPoint *hull,
                                     CarCollisionPoint
                                         grid[CAR_COLLISION_QUAD_COUNT]
                                             [CAR_COLLISION_QUAD_COUNT]) {
    CarCollisionPoint outline[PLAYER_HULL_POINT_COUNT];
    const HullAxes axes = BuildHullAxes(car->bodyPitch, car->bodyYaw, car->bodyRoll);
    s32 index;

    for (index = 0; index < PLAYER_HULL_POINT_COUNT; index++) {
        const LVec transformed = RotateHullPoint(&axes, &hull[index]);
        outline[index].x = WrapSigned16(transformed.x >> 1);
        outline[index].z = WrapSigned16(transformed.z >> 1);
        if (index < CAR_COLLISION_QUAD_COUNT) {
            grid[index][index] = outline[index];
        }
    }

    grid[0][1] = grid[1][0] =
        CarCollisionMidpoint(outline[0], outline[1]);
    grid[0][2] = grid[2][0] = outline[4];
    grid[1][3] = grid[3][1] = outline[5];
    grid[2][3] = grid[3][2] =
        CarCollisionMidpoint(outline[2], outline[3]);
    grid[0][3] = grid[1][2] = grid[2][1] = grid[3][0] =
        CarCollisionMidpoint(outline[4], outline[5]);
}

static void BuildOpponentCollisionSamples(const PlayerCarRuntime *player,
                                          const GameCarRuntime *opponent,
                                          const CarHullPoint *hull,
                                          CarCollisionPoint
                                              corners[CAR_COLLISION_QUAD_COUNT],
                                          CarCollisionPoint samples
                                              [OPPONENT_COLLISION_SAMPLE_COUNT]) {
    const HullAxes axes = BuildHullAxes(opponent->bodyPitch, opponent->bodyYaw, opponent->bodyRoll);
    s32 offsetX = WrapSigned16((u16)opponent->x - (u16)player->x);
    s32 offsetZ = WrapSigned16((u16)opponent->z - (u16)player->z);
    s32 index;

    for (index = 0; index < CAR_COLLISION_QUAD_COUNT; index++) {
        const LVec transformed = RotateHullPoint(&axes, &hull[index]);
        corners[index].x = WrapSigned16(
            (int64_t)(transformed.x >> 1) + offsetX / 2);
        corners[index].z = WrapSigned16(
            (int64_t)(transformed.z >> 1) + offsetZ / 2);
    }

    samples[0] = CarCollisionMidpoint(corners[0], corners[1]);
    samples[1] = CarCollisionMidpoint(corners[0], corners[2]);
    samples[2] = CarCollisionMidpoint(corners[1], corners[3]);
    samples[3] = CarCollisionMidpoint(corners[2], corners[3]);
    samples[4] = CarCollisionMidpoint(samples[0], samples[2]);
    samples[5] = CarCollisionMidpoint(corners[0], samples[1]);
    samples[6] = CarCollisionMidpoint(corners[1], samples[2]);
    samples[7] = CarCollisionMidpoint(corners[2], samples[1]);
    samples[8] = CarCollisionMidpoint(corners[3], samples[2]);
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

static s32 AbsoluteDifference(s32 a, s32 b) {
    int64_t difference = (int64_t)a - b;

    if (difference < 0) {
        difference = -difference;
    }
    return difference > INT32_MAX ? INT32_MAX : (s32)difference;
}

static CarContact FindCandidateContact(
    PlayerCarRuntime *player, const CarCollider *field, s32 count,
    s32 trackLength,
    CarCollisionPoint
        playerGrid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT]) {
    CarContact hit = {0};
    CarCollisionHit quadHit;
    CarCollisionPoint samples[OPPONENT_COLLISION_SAMPLE_COUNT];
    CarCollisionPoint corners[CAR_COLLISION_QUAD_COUNT];
    s32 index;

    for (index = 0; index < count; index++) {
        GameCarRuntime *opponent = field[index].car;
        s32 heightDistance;
        s32 progressDistance;
        s32 lateralDistance;

        if (opponent == NULL || opponent == AsRivalCar(player) ||
            field[index].corners == NULL || opponent->activeFlag == -1) {
            continue;
        }
        progressDistance = WrapSigned32(
            (int64_t)opponent->trackProgress + trackLength);
        progressDistance = WrapSigned32(
            (int64_t)progressDistance - player->trackProgress) %
            trackLength;
        lateralDistance = AbsoluteDifference(opponent->trackLateralOffset,
                                             player->trackLateralOffset);
        heightDistance = AbsoluteDifference(opponent->y, player->y);
        if (opponent->verticalMotionState != player->verticalMotionState &&
            heightDistance >= DIFFERENT_LEVEL_HEIGHT) {
            continue;
        }

        if (lateralDistance >= COLLISION_LATERAL_REACH ||
            (progressDistance >= COLLISION_PROGRESS_REACH &&
             progressDistance <=
                 trackLength - COLLISION_PROGRESS_REACH) ||
            heightDistance >= COLLISION_HEIGHT_REACH) {
            if (lateralDistance < SLIPSTREAM_LATERAL_REACH &&
                progressDistance < SLIPSTREAM_PROGRESS_REACH) {
                s32 remainingDistance = WrapSigned32(
                    (int64_t)SLIPSTREAM_PROGRESS_REACH - progressDistance);

                player->drive.dragScale = WrapSigned32(
                    (int64_t)SLIPSTREAM_PROGRESS_REACH -
                    (remainingDistance >> 2));
            }
            continue;
        }

        if (progressDistance < COLLISION_PROGRESS_REACH &&
            lateralDistance < SLIPSTREAM_LATERAL_REACH) {
            player->drive.dragScale = CLOSE_SLIPSTREAM_DRAG;
        }
        BuildOpponentCollisionSamples(player, opponent, field[index].corners,
                                      corners, samples);
        quadHit = FindPlayerCollisionRegion(playerGrid, corners, samples);
        if (quadHit.region > 0) {
            hit.opponent = opponent;
            hit.region = quadHit.region;
            hit.lateralDistance = lateralDistance;
            return hit;
        }
    }
    return hit;
}

CarContact FindCarContact(PlayerCarRuntime *car,
                          const CarHullPoint hull[PLAYER_HULL_SAMPLE_COUNT],
                          const CarCollider *field, s32 count, s32 trackLength) {
    const CarContact none = {0};
    CarCollisionPoint grid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT];
    if (car == NULL || hull == NULL || field == NULL ||
        count <= 0 || trackLength <= 0) {
        return none;
    }
    BuildPlayerCollisionGrid(car, hull, grid);
    return FindCandidateContact(car, field, count, trackLength, grid);
}
