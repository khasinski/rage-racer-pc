#ifndef RAGE_RAY_GPU_H
#define RAGE_RAY_GPU_H

#include <stddef.h>
#include <stdint.h>

#include "ray_bvh.h"

/* std430-compatible storage-buffer ABI shared by GLSL, SPIR-V and MSL. */
typedef struct RayGpuNode {
    float minimum[4];
    float maximum[4];
    uint32_t childAndRange[4]; /* left, right, first, count */
} RayGpuNode;

typedef struct RayGpuTriangle {
    float vertex0[4];
    float vertex1[4];
    float vertex2[4];
} RayGpuTriangle;

typedef struct RayGpuLayout {
    size_t nodeBytes;
    size_t triangleBytes;
    size_t indexBytes;
} RayGpuLayout;

int RayGpuLayoutForMesh(const RayMesh *mesh, RayGpuLayout *out);
int RayGpuPackMesh(const RayMesh *mesh,
                   RayGpuNode *nodes, size_t nodeCapacity,
                   RayGpuTriangle *triangles, size_t triangleCapacity,
                   uint32_t *indices, size_t indexCapacity);

#endif
