#ifndef GAME_CAR_COLLISION_INTERNAL_H
#define GAME_CAR_COLLISION_INTERNAL_H

#include "game/car.h"

typedef struct CarCollisionHit {
    s32 region;
    s32 sampleIndex;
    s32 quadIndex;
} CarCollisionHit;

enum {
    CAR_COLLISION_QUAD_COUNT = CAR_HULL_CORNER_COUNT,
    LAST_FRONT_COLLISION_REGION = 2,
};

CarCollisionPoint CarCollisionMidpoint(CarCollisionPoint first,
                                       CarCollisionPoint second);
CarCollisionHit FindFirstCarCollisionQuad(
    const CarCollisionPoint
        grid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT],
    const CarCollisionPoint *points, s32 count);

typedef struct CarCollider {
    GameCarRuntime *car;
    const CarHullPoint *corners; /* Four model-specific hull corners. */
} CarCollider;

typedef struct CarContact {
    GameCarRuntime *opponent;
    s32 region;
    s32 lateralDistance;
} CarContact;

/* Returns the first contact and updates this driver's slipstream drag.
 * Does not clear field flags, apply responses or play sounds. */
CarContact FindCarContact(PlayerCarRuntime *car,
                          const CarHullPoint hull[PLAYER_HULL_SAMPLE_COUNT],
                          const CarCollider *field, s32 count, s32 trackLength);

/* Apply an already detected contact without audio, track or race globals. */
void ApplyCarCollision(PlayerCarRuntime *car, GameCarRuntime *opponent,
                        s32 region, int reverse, s32 wrongWayFrames);

typedef struct DriverHull {
    const CarHullPoint *points; /* Six collision-grid samples. */
    const CarHullPoint *corners; /* Four opponent silhouette corners. */
} DriverHull;

typedef struct DriverContact {
    s32 firstRegion;
    s32 secondRegion;
} DriverContact;

/* Read-only pair query: regions may differ or only one may detect contact.
 * Uses each driver's own model hull; does not change slipstream state. */
DriverContact FindDriverContact(const PlayerCarRuntime *first, const DriverHull *firstHull,
                                  const PlayerCarRuntime *second, const DriverHull *secondHull,
                                  s32 trackLength);

/* Human pair response uses snapshots, so exchanging the two arguments (and
 * their detected regions/wrong-way durations) produces the same result.
 * A zero region skips that driver's directional response; an actual pair
 * contact still marks both drivers for the common crash response. */
void ApplyDriverCollision(PlayerCarRuntime *first, PlayerCarRuntime *second,
                           s32 firstRegion, s32 secondRegion, int reverse,
                           s32 firstWrongWay, s32 secondWrongWay);

/* Read-only AI pair query, followed by an explicit pair response. */
s32 FindRivalContact(const GameCarRuntime *car, const CarHullPoint *hull,
                     const GameCarRuntime *other, const CarHullPoint *otherHull,
                     s32 trackLength);
void ApplyRivalCollision(GameCarRuntime *car, GameCarRuntime *other, s32 region);

/* Player-vs-field collision detection and response. Returns the struck hull
 * region (1..4), or zero when no opponent was hit. */
s32 CollidePlayerWithCars(PlayerCarRuntime *car);
/* Test car[index] against the remaining AI slots, pushing apart the first
 * colliding pair. */
s32 CollideRivalCars(s32 index);

#endif
