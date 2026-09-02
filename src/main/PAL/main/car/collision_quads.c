/*
 * Searching a car's hull for the point where another car touches it.
 *
 * The hull is cut into four quadrants and each candidate point is tried
 * against each quadrant in turn. Both the player's collision and the rivals'
 * use this; which quadrant answers is what decides who gets shoved.
 */

#include "game/car_internal.h"

/*
 * Reports which of the four collision quads contains the first candidate
 * point. A miss has region 0 and -1 indices; a hit has region 1..4 and the
 * matching zero-based point and quad indices used by collision tracing.
 */
CarCollisionHit FindFirstCarCollisionQuad(
    const CarCollisionPoint grid[4][4], const CarCollisionPoint *points,
    s32 count) {
    CarCollisionHit hit = {.region = 0, .sampleIndex = -1, .quadIndex = -1};
    s32 sampleIndex;
    s32 quadIndex;

    for (sampleIndex = 0; sampleIndex < count; sampleIndex++) {
        for (quadIndex = 0; quadIndex < 4; quadIndex++) {
            if (IsPointInQuad(
                    GetCarCollisionPointPacked(&grid[quadIndex][2]),
                    GetCarCollisionPointPacked(&grid[quadIndex][3]),
                    GetCarCollisionPointPacked(&grid[quadIndex][0]),
                    GetCarCollisionPointPacked(&grid[quadIndex][1]),
                    GetCarCollisionPointPacked(&points[sampleIndex])) > 0) {
                hit.region = quadIndex + 1;
                hit.sampleIndex = sampleIndex;
                hit.quadIndex = quadIndex;
                return hit;
            }
        }
    }
    return hit;
}
