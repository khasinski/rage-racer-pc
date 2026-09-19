#ifndef RAGE_RAY_GPU_H
#define RAGE_RAY_GPU_H

#include <stddef.h>
#include <stdint.h>

#include "ray_bvh.h"
#include "ray_scene.h"

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

typedef struct RayGpuInstance {
    /* Affine world-to-local rows. The fourth component is the translation. */
    float worldToLocal[3][4];
    /* BLAS root node, node count, instance flags, reserved. */
    uint32_t meshAndFlags[4];
} RayGpuInstance;

typedef struct RayGpuLayout {
    size_t nodeBytes;
    size_t triangleBytes;
    size_t indexBytes;
} RayGpuLayout;

typedef struct RayGpuSceneLayout {
    size_t nodeBytes;
    size_t triangleBytes;
    size_t indexBytes;
    size_t instanceBytes;
    uint32_t nodeCount;
    uint32_t triangleCount;
    uint32_t indexCount;
    uint32_t instanceCount;
} RayGpuSceneLayout;

int RayGpuLayoutForMesh(const RayMesh *mesh, RayGpuLayout *out);
int RayGpuPackMesh(const RayMesh *mesh,
                   RayGpuNode *nodes, size_t nodeCapacity,
                   RayGpuTriangle *triangles, size_t triangleCapacity,
                   uint32_t *indices, size_t indexCapacity);

/* Pack TLAS and each distinct referenced BLAS into shared arrays. Meshes are
 * deduplicated by identity because RayScene keeps immutable mesh pointers. */
int RayGpuLayoutForScene(const RayScene *scene, RayGpuSceneLayout *out);
int RayGpuPackScene(const RayScene *scene, const RayGpuSceneLayout *layout,
                    RayGpuNode *nodes, size_t nodeCapacity,
                    RayGpuTriangle *triangles, size_t triangleCapacity,
                    uint32_t *indices, size_t indexCapacity,
                    RayGpuInstance *instances, size_t instanceCapacity);

#endif
