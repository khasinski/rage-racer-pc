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
#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/track.h"
#include "psyq/gte.h"

enum {
    /* Cars further apart than this along or across the track cannot touch. */
    COLLISION_TRACK_REACH = 200,
    COLLISION_LATERAL_REACH = 100,
    CAR_COLLISION_SAMPLE_COUNT = 5,
};

/* Transforms the four hull corners into the frame of `source`, offset by
 * however far `source` is from the car the frame belongs to. */
static void TransformCarHull(const GameCarRuntime *source,
                             CarCollisionPoint *corners, s32 offsetX,
                             s32 offsetZ) {
    SVECTOR rotation;
    SVECTOR input;
    VECTOR transformed;
    Matrix matrix;
    s32 corner;

    rotation.vx = WrapSigned16(source->bodyPitch);
    rotation.vy = WrapSigned16(source->bodyYaw);
    rotation.vz = WrapSigned16(source->bodyRoll);
    rotation.pad = 0;
    RotMatrix(&rotation, &matrix);
    SetRotMatrix(&matrix);
    input.vy = 0;
    input.pad = 0;
    for (corner = 0; corner < CAR_COLLISION_QUAD_COUNT; corner++) {
        input.vx = g_CarCollisionCorners[corner].x;
        input.vz = g_CarCollisionCorners[corner].z;
        ApplyRotMatrix(&input, &transformed);
        corners[corner].x = WrapSigned16(
            (int64_t)(transformed.vx >> 2) + offsetX);
        corners[corner].z = WrapSigned16(
            (int64_t)(transformed.vz >> 2) + offsetZ);
    }
}

static s16 AverageCoordinate(s16 first, s16 second) {
    return (s16)(((s32)first + second) / 2);
}

/*
 * Cuts the hull into four quadrants: grid[q] is the quadrant whose own corner
 * is corners[q], and whose other three points are the two edge midpoints
 * beside it and the centre of the car.
 *
 * Every midpoint rounds toward zero, including negative coordinates. Keeping
 * that rule in AverageCoordinate replaces the recovered mix of signed shifts,
 * unsigned shifts and divisions without changing the resulting halfwords.
 */
static void BuildCollisionQuads(const CarCollisionPoint *corners,
                                CarCollisionPoint
                                    grid[CAR_COLLISION_QUAD_COUNT]
                                        [CAR_COLLISION_QUAD_COUNT]) {
    CarCollisionPoint average01;
    CarCollisionPoint average02;
    CarCollisionPoint average13;
    CarCollisionPoint average23;
    CarCollisionPoint center;
    s32 corner;

    for (corner = 0; corner < 4; corner++) {
        grid[corner][corner] = corners[corner];
    }

    average01.x = AverageCoordinate(corners[0].x, corners[1].x);
    average01.z = AverageCoordinate(corners[0].z, corners[1].z);
    average02.x = AverageCoordinate(corners[0].x, corners[2].x);
    average02.z = AverageCoordinate(corners[0].z, corners[2].z);
    average13.x = AverageCoordinate(corners[1].x, corners[3].x);
    average13.z = AverageCoordinate(corners[1].z, corners[3].z);
    average23.x = AverageCoordinate(corners[2].x, corners[3].x);
    average23.z = AverageCoordinate(corners[2].z, corners[3].z);
    center.x = AverageCoordinate(average01.x, average23.x);
    center.z = AverageCoordinate(average01.z, average23.z);

    grid[1][0] = grid[0][1] = average01;
    grid[2][0] = grid[0][2] = average02;
    grid[3][1] = grid[1][3] = average13;
    grid[3][2] = grid[2][3] = average23;
    grid[3][0] = grid[2][1] = grid[1][2] = grid[0][3] = center;
}

/* The four edge midpoints of the other car's hull, and its centre. */
static void BuildHullSamples(const CarCollisionPoint *corners,
                             CarCollisionPoint *samples) {
    samples[0].x = AverageCoordinate(corners[0].x, corners[1].x);
    samples[0].z = AverageCoordinate(corners[0].z, corners[1].z);
    samples[1].x = AverageCoordinate(corners[0].x, corners[2].x);
    samples[1].z = AverageCoordinate(corners[0].z, corners[2].z);
    samples[2].x = AverageCoordinate(corners[1].x, corners[3].x);
    samples[2].z = AverageCoordinate(corners[1].z, corners[3].z);
    samples[3].x = AverageCoordinate(corners[2].x, corners[3].x);
    samples[3].z = AverageCoordinate(corners[2].z, corners[3].z);
    samples[4].x = AverageCoordinate(samples[0].x, samples[2].x);
    samples[4].z = AverageCoordinate(samples[0].z, samples[2].z);
}

/* How hard the shove is: the speed difference, a thirty-second of it, rounded
 * toward zero. */
static s16 ScaledSpeedDelta(s32 faster, s32 slower) {
    s16 delta = WrapSigned16((u16)faster - (u16)slower);

    return delta / 32;
}

/*
 * Hit in one of the two front quadrants and the other car is shoved away; hit
 * behind and this one is, the other way. Whichever was not shoved loses a
 * tenth of its acceleration.
 */
static void ShoveApart(GameCarRuntime *car, GameCarRuntime *other, s32 hit) {
    s16 pushX = ScaledSpeedDelta(other->worldVelocityX, car->worldVelocityX);
    s16 pushZ = ScaledSpeedDelta(other->worldVelocityZ, car->worldVelocityZ);

    if (hit <= LAST_FRONT_COLLISION_REGION) {
        SetCarKnockback(car, 0, 0, CAR_KNOCKBACK_VECTOR_MODE);
        SetCarKnockback(other, pushX, pushZ, CAR_KNOCKBACK_VECTOR_MODE);
        car->acceleration = WrapSigned32(
            (int64_t)car->acceleration * 90) / 100;
    } else {
        SetCarKnockback(car, -pushX, -pushZ, CAR_KNOCKBACK_VECTOR_MODE);
        SetCarKnockback(other, 0, 0, CAR_KNOCKBACK_VECTOR_MODE);
        other->acceleration = WrapSigned32(
            (int64_t)other->acceleration * 90) / 100;
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
        (other->verticalMotionState != car->verticalMotionState) ||
        g_TrackLength <= 0) {
        return 0;
    }
    progressDelta = WrapSigned32(
        (int64_t)other->trackProgress + g_TrackLength);
    progressDelta = WrapSigned32(
        (int64_t)progressDelta - car->trackProgress) % g_TrackLength;
    distance = WrapSigned32((int64_t)other->trackLateralOffset -
                            car->trackLateralOffset);
    if (distance < 0) {
        distance = WrapSigned32(-(int64_t)distance);
    }
    return (distance < COLLISION_LATERAL_REACH) &&
           ((progressDelta < COLLISION_TRACK_REACH) ||
            (g_TrackLength - COLLISION_TRACK_REACH < progressDelta));
}

s32 CollideRivalCars(GameCarRuntime *car, s32 index) {
    CarCollisionPoint quads[CAR_COLLISION_QUAD_COUNT]
                           [CAR_COLLISION_QUAD_COUNT];
    CarCollisionPoint carCorners[CAR_COLLISION_QUAD_COUNT];
    CarCollisionPoint otherCorners[CAR_COLLISION_QUAD_COUNT];
    CarCollisionPoint samples[CAR_COLLISION_SAMPLE_COUNT];
    s32 nextIndex;
    int hullBuilt = 0;

    if (index < 0 || index >= RACE_CAR_SLOT_COUNT - 1) {
        return 0;
    }

    for (nextIndex = index + 1;
         nextIndex < RACE_CAR_SLOT_COUNT;
         nextIndex++) {
        GameCarRuntime *other = &g_Cars[nextIndex];

        if (WithinCollisionReach(car, other)) {
            CarCollisionHit collision;

            if (!hullBuilt) {
                TransformCarHull(car, carCorners, 0, 0);
                BuildCollisionQuads(carCorners, quads);
                hullBuilt = 1;
            }
            TransformCarHull(other, otherCorners,
                             WrapSigned16((u16)other->x - (u16)car->x),
                             WrapSigned16((u16)other->z - (u16)car->z));
            BuildHullSamples(otherCorners, samples);

            /* Corners first, then the edge and centre points: the cheapest
               test that can hit, first. */
            collision =
                FindFirstCarCollisionQuad(quads, otherCorners,
                                          CAR_COLLISION_QUAD_COUNT);
            if (collision.region <= 0) {
                collision = FindFirstCarCollisionQuad(
                    quads, samples, CAR_COLLISION_SAMPLE_COUNT);
            }
            if (collision.region > 0) {
                ShoveApart(car, other, collision.region);
                return collision.region;
            }
        }
    }
    return 0;
}
