/*
 * Searching a car's hull for the point where another car touches it.
 *
 * The hull is cut into four quadrants and each candidate point is tried
 * against each quadrant in turn. Both the player's collision and the rivals'
 * use this; which quadrant answers is what decides who gets shoved.
 */

#include "game/car.h"

/*
 * Which of the four collision quads of a car's hull the first of `points` falls
 * inside, 1..4, or 0 when none of them does. `sample` and `quad` report where
 * it stopped, which is what the collision trace prints.
 */
s32 FirstQuadHit(CarCollisionPoint grid[4][4],
                 const CarCollisionPoint *points, s32 count,
                 s32 *sample, s32 *quad) {
    s32 sampleIndex;
    s32 quadIndex = 0;

    for (sampleIndex = 0; sampleIndex < count; sampleIndex++) {
        for (quadIndex = 0; quadIndex < 4; quadIndex++) {
            if (IsPointInQuad(
                    GetCarCollisionPointPacked(&grid[quadIndex][2]),
                    GetCarCollisionPointPacked(&grid[quadIndex][3]),
                    GetCarCollisionPointPacked(&grid[quadIndex][0]),
                    GetCarCollisionPointPacked(&grid[quadIndex][1]),
                    GetCarCollisionPointPacked(&points[sampleIndex])) > 0) {
                *sample = sampleIndex;
                *quad = quadIndex;
                return quadIndex + 1;
            }
        }
    }
    *sample = sampleIndex;
    *quad = quadIndex;
    return 0;
}
