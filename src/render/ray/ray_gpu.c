#include "ray_gpu.h"

#include <stdint.h>

_Static_assert(sizeof(RayGpuNode) == 48, "ray node GPU ABI");
_Static_assert(sizeof(RayGpuTriangle) == 48, "ray triangle GPU ABI");

int RayGpuLayoutForMesh(const RayMesh *mesh, RayGpuLayout *out) {
    RayGpuLayout layout;
    if (mesh == NULL || out == NULL || mesh->nodeCount == 0 ||
        mesh->triangleCount == 0 || mesh->nodes == NULL ||
        mesh->triangles == NULL || mesh->indices == NULL) return 0;
#if SIZE_MAX <= UINT32_MAX
    if (mesh->nodeCount > SIZE_MAX / sizeof(RayGpuNode) ||
        mesh->triangleCount > SIZE_MAX / sizeof(RayGpuTriangle) ||
        mesh->triangleCount > SIZE_MAX / sizeof(uint32_t)) return 0;
#endif
    layout.nodeBytes = (size_t)mesh->nodeCount * sizeof(RayGpuNode);
    layout.triangleBytes =
        (size_t)mesh->triangleCount * sizeof(RayGpuTriangle);
    layout.indexBytes = (size_t)mesh->triangleCount * sizeof(uint32_t);
    *out = layout;
    return 1;
}

int RayGpuPackMesh(const RayMesh *mesh,
                   RayGpuNode *nodes, size_t nodeCapacity,
                   RayGpuTriangle *triangles, size_t triangleCapacity,
                   uint32_t *indices, size_t indexCapacity) {
    if (mesh == NULL || nodes == NULL || triangles == NULL || indices == NULL ||
        nodeCapacity < mesh->nodeCount ||
        triangleCapacity < mesh->triangleCount ||
        indexCapacity < mesh->triangleCount) return 0;
    for (uint32_t index = 0; index < mesh->nodeCount; ++index) {
        const RayBvhNode *source = &mesh->nodes[index];
        RayGpuNode *target = &nodes[index];
        target->minimum[0] = source->bounds.min.x;
        target->minimum[1] = source->bounds.min.y;
        target->minimum[2] = source->bounds.min.z;
        target->minimum[3] = 0.0f;
        target->maximum[0] = source->bounds.max.x;
        target->maximum[1] = source->bounds.max.y;
        target->maximum[2] = source->bounds.max.z;
        target->maximum[3] = 0.0f;
        target->childAndRange[0] = source->left;
        target->childAndRange[1] = source->right;
        target->childAndRange[2] = source->first;
        target->childAndRange[3] = source->count;
    }
    for (uint32_t index = 0; index < mesh->triangleCount; ++index) {
        const RayTriangle *source = &mesh->triangles[index];
        RayGpuTriangle *target = &triangles[index];
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float *vertex = &source->vertex[axis].x;
            float *packed = axis == 0 ? target->vertex0 :
                            axis == 1 ? target->vertex1 : target->vertex2;
            packed[0] = vertex[0];
            packed[1] = vertex[1];
            packed[2] = vertex[2];
            packed[3] = 0.0f;
        }
        indices[index] = mesh->indices[index];
    }
    return 1;
}
