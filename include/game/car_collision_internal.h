#ifndef GAME_CAR_COLLISION_INTERNAL_H
#define GAME_CAR_COLLISION_INTERNAL_H

#include "game/car.h"

typedef struct CarCollisionHit {
    s32 region;
    s32 sampleIndex;
    s32 quadIndex;
} CarCollisionHit;

enum {
    CAR_COLLISION_QUAD_COUNT = 4,
    LAST_FRONT_COLLISION_REGION = 2,
};

CarCollisionHit FindFirstCarCollisionQuad(
    const CarCollisionPoint
        grid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT],
    const CarCollisionPoint *points, s32 count);

/* Player-vs-field collision detection and response. Returns the struck hull
 * region (1..4), or zero when no opponent was hit. */
s32 CollidePlayerWithCars(PlayerCarRuntime *car);
/* Test car[index] against the remaining AI slots, pushing apart the first
 * colliding pair. */
s32 CollideRivalCars(s32 index);

#endif
