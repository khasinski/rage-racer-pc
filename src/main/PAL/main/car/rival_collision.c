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
#include "game/hull_rotation.h"
#include <stddef.h>

enum {
    /* Cars further apart than this along or across the track cannot touch. */
    COLLISION_TRACK_REACH = 200,
    COLLISION_LATERAL_REACH = 100,
    CAR_COLLISION_SAMPLE_COUNT = 5,
    COLLISION_PUSH_DIVISOR = 32,
    UNSHOVED_ACCELERATION_PERCENT = 90,
};

/* Transforms the four hull corners into the frame of `source`, offset by
 * however far `source` is from the car the frame belongs to. */
static void TransformCarHull(const GameCarRuntime *source,
                             CarCollisionPoint *corners, s32 offsetX,
                             s32 offsetZ, const CarHullPoint *hull) {
    const HullAxes axes = BuildHullAxes(source->bodyPitch, source->bodyYaw, source->bodyRoll);
    s32 corner;
    for (corner = 0; corner < CAR_COLLISION_QUAD_COUNT; corner++) {
        const LVec transformed = RotateHullPoint(&axes, &hull[corner]);
        corners[corner].x = WrapSigned16(
            (int64_t)(transformed.x >> 2) + offsetX);
        corners[corner].z = WrapSigned16(
            (int64_t)(transformed.z >> 2) + offsetZ);
    }
}

/*
 * Cuts the hull into four quadrants: grid[q] is the quadrant whose own corner
 * is corners[q], and whose other three points are the two edge midpoints
 * beside it and the centre of the car.
 *
 * Every midpoint rounds toward zero, including negative coordinates. Keeping
 * that rule in CarCollisionMidpoint replaces the recovered mix of signed shifts,
 * unsigned shifts and divisions without changing the resulting halfwords.
 */
static void BuildCollisionQuads(const CarCollisionPoint *corners,
                                CarCollisionPoint
                                    grid[CAR_COLLISION_QUAD_COUNT]
                                        [CAR_COLLISION_QUAD_COUNT]) {
    CarCollisionPoint midpoint01;
    CarCollisionPoint midpoint02;
    CarCollisionPoint midpoint13;
    CarCollisionPoint midpoint23;
    CarCollisionPoint center;
    s32 corner;

    for (corner = 0; corner < CAR_COLLISION_QUAD_COUNT; corner++) {
        grid[corner][corner] = corners[corner];
    }

    midpoint01 = CarCollisionMidpoint(corners[0], corners[1]);
    midpoint02 = CarCollisionMidpoint(corners[0], corners[2]);
    midpoint13 = CarCollisionMidpoint(corners[1], corners[3]);
    midpoint23 = CarCollisionMidpoint(corners[2], corners[3]);
    center = CarCollisionMidpoint(midpoint01, midpoint23);

    grid[1][0] = grid[0][1] = midpoint01;
    grid[2][0] = grid[0][2] = midpoint02;
    grid[3][1] = grid[1][3] = midpoint13;
    grid[3][2] = grid[2][3] = midpoint23;
    grid[3][0] = grid[2][1] = grid[1][2] = grid[0][3] = center;
}

/* The four edge midpoints of the other car's hull, and its centre. */
static void BuildHullSamples(const CarCollisionPoint *corners,
                             CarCollisionPoint *samples) {
    samples[0] = CarCollisionMidpoint(corners[0], corners[1]);
    samples[1] = CarCollisionMidpoint(corners[0], corners[2]);
    samples[2] = CarCollisionMidpoint(corners[1], corners[3]);
    samples[3] = CarCollisionMidpoint(corners[2], corners[3]);
    samples[4] = CarCollisionMidpoint(samples[0], samples[2]);
}

/* How hard the shove is: the speed difference, a thirty-second of it, rounded
 * toward zero. */
static s16 ScaledSpeedDelta(s32 faster, s32 slower) {
    s16 delta = WrapSigned16((u16)faster - (u16)slower);

    return delta / COLLISION_PUSH_DIVISOR;
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
        SetCarCollisionKnockback(car, 0, 0);
        SetCarCollisionKnockback(other, pushX, pushZ);
        car->acceleration = WrapSigned32(
            (int64_t)car->acceleration * UNSHOVED_ACCELERATION_PERCENT) / 100;
    } else {
        SetCarCollisionKnockback(car, -pushX, -pushZ);
        SetCarCollisionKnockback(other, 0, 0);
        other->acceleration = WrapSigned32(
            (int64_t)other->acceleration * UNSHOVED_ACCELERATION_PERCENT) /
            100;
    }
    car->collisionFlag = 1;
    other->collisionFlag = 1;
}

/* Is the other car close enough along and across the track to be worth
 * testing, and on the same level of it? */
static int WithinCollisionReach(const GameCarRuntime *car,
                                const GameCarRuntime *other, s32 trackLength) {
    s32 progressDelta;
    s32 distance;

    if ((other->activeFlag == -1) ||
        (other->verticalMotionState != car->verticalMotionState) ||
        trackLength <= 0) {
        return 0;
    }
    progressDelta = WrapSigned32(
        (int64_t)other->trackProgress + trackLength);
    progressDelta = WrapSigned32(
        (int64_t)progressDelta - car->trackProgress) % trackLength;
    distance = WrapSigned32((int64_t)other->trackLateralOffset -
                            car->trackLateralOffset);
    if (distance < 0) {
        distance = WrapSigned32(-(int64_t)distance);
    }
    return (distance < COLLISION_LATERAL_REACH) &&
           ((progressDelta < COLLISION_TRACK_REACH) ||
            (trackLength - COLLISION_TRACK_REACH < progressDelta));
}

s32 FindRivalContact(const GameCarRuntime *car, const CarHullPoint *hull,
                     const GameCarRuntime *other, const CarHullPoint *otherHull,
                     s32 trackLength) {
    CarCollisionPoint quads[4][4], corners[4], otherCorners[4], samples[5];
    if (car == NULL || other == NULL || car == other || hull == NULL ||
        otherHull == NULL || car->activeFlag == -1 ||
        !WithinCollisionReach(car, other, trackLength)) return 0;
    TransformCarHull(car, corners, 0, 0, hull);
    BuildCollisionQuads(corners, quads);
    TransformCarHull(other, otherCorners,
                     WrapSigned16((u16)other->x - (u16)car->x),
                     WrapSigned16((u16)other->z - (u16)car->z), otherHull);
    BuildHullSamples(otherCorners, samples);
    CarCollisionHit hit = FindFirstCarCollisionQuad(quads, otherCorners, 4);
    if (hit.region <= 0) hit = FindFirstCarCollisionQuad(quads, samples, 5);
    return hit.region;
}

void ApplyRivalCollision(GameCarRuntime *car, GameCarRuntime *other, s32 region) {
    if (car == NULL || other == NULL || car == other || region < 1 || region > 4) return;
    ShoveApart(car, other, region);
}
