#ifndef RAGE_RENDER_TRIANGLE_GEOMETRY_H
#define RAGE_RENDER_TRIANGLE_GEOMETRY_H

#include <math.h>
#include "render_world.h"

/* Position-only triangle shape, independent of view and shading payload.
 * Keep the unnormalised cross product for both flat normals and displacement.
 * Degenerate triangles are retained; callers choose their fallback normal. */
typedef struct RageTriangleGeometry {
    float nx, ny, nz, length;
    int prepared;
} RageTriangleGeometry;

static inline RageTriangleGeometry RenderTriangleGeometry(
    const RageRenderVec3 positions[3]) {
    float ax = positions[1].x - positions[0].x;
    float ay = positions[1].y - positions[0].y;
    float az = positions[1].z - positions[0].z;
    float bx = positions[2].x - positions[0].x;
    float by = positions[2].y - positions[0].y;
    float bz = positions[2].z - positions[0].z;
    float nx = ay * bz - az * by;
    float ny = az * bx - ax * bz;
    float nz = ax * by - ay * bx;
    RageTriangleGeometry result = {
        nx, ny, nz, sqrtf(nx * nx + ny * ny + nz * nz), 1};
    return result;
}

/* Retail road-paint shape heuristic in world units. Evaluate after instance
 * scaling and terrain snapping, never blindly cache it in source-mesh space. */
static inline int RenderTriangleIsRoadDecal(
    const RageRenderVec3 positions[3], RageTriangleGeometry *geometry) {
    float edge[3];
    for (unsigned corner = 0; corner < 3; ++corner) {
        unsigned next = (corner + 1) % 3;
        float x = positions[next].x - positions[corner].x;
        float y = positions[next].y - positions[corner].y;
        float z = positions[next].z - positions[corner].z;
        edge[corner] = sqrtf(x * x + y * y + z * z);
    }
    float shortest = fminf(edge[0], fminf(edge[1], edge[2]));
    float longest = fmaxf(edge[0], fmaxf(edge[1], edge[2]));
    if (shortest > 16.0f || longest < 64.0f || longest < shortest * 8.0f)
        return 0;
    if (!geometry->prepared) *geometry = RenderTriangleGeometry(positions);
    return geometry->length > 0.0f &&
           fabsf(geometry->ny) >= geometry->length * 0.85f;
}

#endif
