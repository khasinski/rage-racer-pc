#ifndef RAGE_RAY_BVH_H
#define RAGE_RAY_BVH_H

#include <stddef.h>

#include "ray.h"

typedef struct RayBvhNode {
    RayBounds bounds;
    uint32_t first;
    uint32_t count;
    uint32_t left;
    uint32_t right;
} RayBvhNode;

typedef struct RayMesh {
    RayTriangle *triangles;
    uint32_t *indices;
    RayBvhNode *nodes;
    uint32_t triangleCount;
    uint32_t nodeCount;
} RayMesh;

/* Initialize with {0}. Build owns a copy and replaces an existing mesh only
 * after the complete BVH succeeds. A failed build leaves the old mesh intact. */
int RayMeshBuild(RayMesh *mesh, const RayTriangle *triangles, size_t count);
void RayMeshRelease(RayMesh *mesh);
int RayMeshTraceClosest(const RayMesh *mesh, const Ray *ray,
                        uint32_t flags, RayHit *hit);
int RayMeshTraceAny(const RayMesh *mesh, const Ray *ray, uint32_t flags);
int RayMeshBounds(const RayMesh *mesh, RayBounds *out);

#endif
