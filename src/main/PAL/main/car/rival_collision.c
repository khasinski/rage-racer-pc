/*
 * Two rival cars hitting each other.
 *
 * The car's hull is transformed into its own frame and cut into four
 * quadrants. The other car's hull is transformed into the same frame, and
 * whichever quadrant its corners or its edge midpoints land in decides who
 * gets shoved: hit in front and the other car is pushed away, hit behind and
 * this one is, the other way.
 *
 * The player's own version lives in player_car_collision.c and works over a
 * six-point hull; the two share the quadrant search.
 */

#include "game/car.h"
#include "game/track.h"
#include "psyq/gte.h"

/* Cars further apart than this along or across the track cannot touch. */
#define COLLISION_TRACK_REACH 200
#define COLLISION_LATERAL_REACH 100

/* Transforms the four hull corners into the frame of `source`, offset by
 * however far `source` is from the car the frame belongs to. */
static void TransformCarHull(const GameCarRuntime *source,
                             CarCollisionPoint *corners, s32 offsetX,
                             s32 offsetZ) {
    s16 rotation[4];
    s32 transformed[3];
    Matrix matrix;
    s32 corner;

    rotation[0] = (u16)source->bodyPitch;
    rotation[2] = (u16)source->bodyRoll;
    rotation[1] = (u16)source->bodyYaw;
    RotMatrix(rotation, &matrix);
    SetRotMatrix(&matrix);
    for (corner = 0; corner < 4; corner++) {
        rotation[0] = g_CarCollisionCorners[corner].x;
        rotation[2] = g_CarCollisionCorners[corner].z;
        rotation[1] = 0;
        TransformCollisionVector(rotation, transformed);
        corners[corner].x = (transformed[0] >> 2) + offsetX;
        corners[corner].z = (transformed[2] >> 2) + offsetZ;
    }
}

/*
 * Cuts the hull into four quadrants: grid[q] is the quadrant whose own corner
 * is corners[q], and whose other three points are the two edge midpoints
 * beside it and the centre of the car.
 *
 * The halving is written out rather than divided because the widths it runs
 * at are not all the same, and the mixed signedness is what the console did.
 */
static void BuildCollisionQuads(const CarCollisionPoint *corners,
                                CarCollisionPoint grid[4][4]) {
    s32 average01X;
    s32 average01Z;
    s32 average23X;
    s32 average23Z;
    u32 average02X;
    u32 average02Z;
    u32 average13X;
    u32 average13Z;
    u32 centerX;
    u32 centerZ;
    s32 corner;

    for (corner = 0; corner < 4; corner++) {
        grid[corner][corner] = corners[corner];
    }

    average02X = corners[0].x + corners[2].x;
    average02X += average02X >> 31;
    average02X >>= 1;
    average02Z = corners[0].z + corners[2].z;
    average02Z += average02Z >> 31;
    average02Z >>= 1;
    average13X = corners[1].x + corners[3].x;
    average13X += average13X >> 31;
    average13X >>= 1;
    average13Z = corners[1].z + corners[3].z;
    average13Z += average13Z >> 31;
    average13Z >>= 1;

    average01X = corners[0].x + corners[1].x;
    average01X += (u32)average01X >> 31;
    average01X >>= 1;
    average23X = corners[2].x + corners[3].x;
    average23X /= 2;
    centerX = (s16)average01X + (s16)average23X;
    centerX += centerX >> 31;
    centerX >>= 1;

    average01Z = corners[0].z + corners[1].z;
    average01Z += (u32)average01Z >> 31;
    average01Z >>= 1;
    average23Z = corners[2].z + corners[3].z;
    average23Z /= 2;
    centerZ = (s16)average01Z + (s16)average23Z;
    centerZ += centerZ >> 31;
    centerZ >>= 1;

    grid[1][0].x = grid[0][1].x = average01X;
    grid[1][0].z = grid[0][1].z = average01Z;
    grid[2][0].x = grid[0][2].x = average02X;
    grid[2][0].z = grid[0][2].z = average02Z;
    grid[3][1].x = grid[1][3].x = average13X;
    grid[3][1].z = grid[1][3].z = average13Z;
    grid[3][2].x = grid[2][3].x = average23X;
    grid[3][2].z = grid[2][3].z = average23Z;
    grid[3][0].x = grid[2][1].x = grid[1][2].x = grid[0][3].x = centerX;
    grid[3][0].z = grid[2][1].z = grid[1][2].z = grid[0][3].z = centerZ;
}

/* The four edge midpoints of the other car's hull, and its centre. */
static void BuildHullSamples(const CarCollisionPoint *corners,
                             CarCollisionPoint *samples) {
    samples[0].x = (corners[0].x + corners[1].x) / 2;
    samples[0].z = (corners[0].z + corners[1].z) / 2;
    samples[1].x = (corners[0].x + corners[2].x) / 2;
    samples[1].z = (corners[0].z + corners[2].z) / 2;
    samples[2].x = (corners[1].x + corners[3].x) / 2;
    samples[2].z = (corners[1].z + corners[3].z) / 2;
    samples[3].x = (corners[2].x + corners[3].x) / 2;
    samples[3].z = (corners[2].z + corners[3].z) / 2;
    samples[4].x = (samples[0].x + samples[2].x) / 2;
    samples[4].z = (samples[0].z + samples[2].z) / 2;
}

/* How hard the shove is: the speed difference, a thirty-second of it, rounded
 * toward zero. */
static s16 ScaledSpeedDelta(s32 faster, s32 slower) {
    s32 delta = (s16)((u16)faster - (u16)slower);

    if (delta < 0) {
        delta += 31;
    }
    return (s16)(delta >> 5);
}

/*
 * Hit in one of the two front quadrants and the other car is shoved away; hit
 * behind and this one is, the other way. Whichever was not shoved loses a
 * tenth of its acceleration.
 */
static void ShoveApart(GameCarRuntime *car, GameCarRuntime *other, s32 hit) {
    s16 pushX = ScaledSpeedDelta(other->worldVelocityX, car->worldVelocityX);
    s16 pushZ = ScaledSpeedDelta(other->worldVelocityZ, car->worldVelocityZ);

    if (hit < 3) {
        SetCarKnockback(car, 0, 0, 4);
        SetCarKnockback(other, pushX, pushZ, 4);
        car->acceleration = (car->acceleration * 90) / 100;
    } else {
        SetCarKnockback(car, -pushX, -pushZ, 4);
        SetCarKnockback(other, 0, 0, 4);
        other->acceleration = (other->acceleration * 90) / 100;
    }
    car->collisionFlag = 1;
    other->collisionFlag = 1;
}

/* Is the other car close enough along and across the track to be worth
 * testing, and on the same level of it? */
static int WithinCollisionReach(const GameCarRuntime *car,
                                const GameCarRuntime *other) {
    s32 progressDelta;
    s32 distance;

    if ((other->activeFlag == -1) ||
        (other->verticalMotionState != car->verticalMotionState)) {
        return 0;
    }
    progressDelta = (other->trackProgress + g_TrackLength - car->trackProgress) %
                    g_TrackLength;
    distance = other->trackLateralOffset - car->trackLateralOffset;
    if (distance < 0) {
        distance = -distance;
    }
    return (distance < COLLISION_LATERAL_REACH) &&
           ((progressDelta < COLLISION_TRACK_REACH) ||
            (g_TrackLength - COLLISION_TRACK_REACH < progressDelta));
}

s32 CollideRivalCars(GameCarRuntime *car, s32 index) {
    CarCollisionPoint quads[4][4];
    CarCollisionPoint carCorners[4];
    CarCollisionPoint otherCorners[4];
    CarCollisionPoint samples[5];
    GameCarRuntime *other = &g_Cars[index + 1];
    s32 nextIndex = index + 1;
    s32 hit = 0;

    while (nextIndex < 11) {
        if (WithinCollisionReach(car, other)) {
            s32 sample;
            s32 quad;

            TransformCarHull(car, carCorners, 0, 0);
            BuildCollisionQuads(carCorners, quads);
            TransformCarHull(other, otherCorners,
                             (s16)((u16)other->x - (u16)car->x),
                             (s16)((u16)other->z - (u16)car->z));
            BuildHullSamples(otherCorners, samples);

            /* Corners first, then the edge and centre points: the cheapest
               test that can hit, first. */
            hit = FirstQuadHit(quads, otherCorners, 4, &sample, &quad);
            if (hit <= 0) {
                hit = FirstQuadHit(quads, samples, 5, &sample, &quad);
            }
            if (hit > 0) {
                break;
            }
        }
        other++;
        nextIndex++;
    }

    if (hit > 0) {
        ShoveApart(car, other, hit);
    }
    return hit;
}
