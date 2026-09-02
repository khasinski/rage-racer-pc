/*
 * Searching a car's hull for the point where another car touches it.
 *
 * The hull is cut into four quadrants and each candidate point is tried
 * against each quadrant in turn. Both the player's collision and the rivals'
 * use this; which quadrant answers is what decides who gets shoved.
 */

#include "game/car_internal.h"
#include "psyq/gte.h"

static int64_t CollisionQuadAreaTwice(const CarCollisionPoint quad[4]) {
    static const u8 order[4] = {2, 3, 1, 0};
    int64_t area = 0;
    s32 edge;

    for (edge = 0; edge < 4; edge++) {
        const CarCollisionPoint *from = &quad[order[edge]];
        const CarCollisionPoint *to = &quad[order[(edge + 1) & 3]];

        area += (int64_t)from->x * to->z - (int64_t)from->z * to->x;
    }
    return area;
}

static int IsPointInsideCollisionQuad(const CarCollisionPoint quad[4],
                                      const CarCollisionPoint *point) {
    s32 p0 = GetCarCollisionPointPacked(&quad[2]);
    s32 p1 = GetCarCollisionPointPacked(&quad[3]);
    s32 p2 = GetCarCollisionPointPacked(&quad[0]);
    s32 p3 = GetCarCollisionPointPacked(&quad[1]);
    s32 packedPoint = GetCarCollisionPointPacked(point);

    return CollisionQuadAreaTwice(quad) != 0 &&
           NormalClip(p0, p1, packedPoint) >= 0 &&
           NormalClip(p1, p3, packedPoint) >= 0 &&
           NormalClip(p3, p2, packedPoint) >= 0 &&
           NormalClip(p2, p0, packedPoint) >= 0;
}

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
            if (IsPointInsideCollisionQuad(grid[quadIndex],
                                           &points[sampleIndex])) {
                hit.region = quadIndex + 1;
                hit.sampleIndex = sampleIndex;
                hit.quadIndex = quadIndex;
                return hit;
            }
        }
    }
    return hit;
}
