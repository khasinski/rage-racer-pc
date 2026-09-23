#include "ray_gpu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(RayGpuNode) == 48, "ray node GPU ABI");
_Static_assert(sizeof(RayGpuTriangle) == 48, "ray triangle GPU ABI");
_Static_assert(sizeof(RayGpuInstance) == 64, "ray instance GPU ABI");

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

static int AddSize(size_t *total, size_t count, size_t itemSize) {
    if (count > SIZE_MAX / itemSize || *total > SIZE_MAX - count * itemSize)
        return 0;
    *total += count * itemSize;
    return 1;
}

static int SceneCounts(const RayScene *scene, RayGpuSceneLayout *out) {
    RayGpuSceneLayout layout = {0};
    if (scene == NULL || out == NULL || scene->nodeCount == 0 ||
        scene->instanceCount == 0 || scene->nodes == NULL ||
        scene->indices == NULL || scene->instances == NULL)
        return 0;
    layout.nodeCount = scene->nodeCount;
    layout.indexCount = scene->instanceCount;
    layout.instanceCount = scene->instanceCount;
    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        const RayMesh *mesh = scene->instances[i].mesh;
        int first = 1;
        if (mesh == NULL || mesh->nodeCount == 0 || mesh->triangleCount == 0)
            return 0;
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (scene->instances[prior].mesh == mesh) {
                first = 0;
                break;
            }
        }
        if (!first) continue;
        if (layout.nodeCount > UINT32_MAX - mesh->nodeCount ||
            layout.triangleCount > UINT32_MAX - mesh->triangleCount ||
            layout.indexCount > UINT32_MAX - mesh->triangleCount)
            return 0;
        layout.nodeCount += mesh->nodeCount;
        layout.triangleCount += mesh->triangleCount;
        layout.indexCount += mesh->triangleCount;
    }
    if (!AddSize(&layout.nodeBytes, layout.nodeCount, sizeof(RayGpuNode)) ||
        !AddSize(&layout.triangleBytes, layout.triangleCount,
                 sizeof(RayGpuTriangle)) ||
        !AddSize(&layout.indexBytes, layout.indexCount, sizeof(uint32_t)) ||
        !AddSize(&layout.instanceBytes, layout.instanceCount,
                 sizeof(RayGpuInstance)))
        return 0;
    *out = layout;
    return 1;
}

int RayGpuLayoutForScene(const RayScene *scene, RayGpuSceneLayout *out) {
    if (out != NULL) *out = (RayGpuSceneLayout){0};
    return SceneCounts(scene, out);
}

static void PackNode(RayGpuNode *out, const RayBvhNode *source,
                     uint32_t nodeOffset, uint32_t firstOffset) {
    *out = (RayGpuNode){
        .minimum = {source->bounds.min.x, source->bounds.min.y,
                    source->bounds.min.z, 0.0f},
        .maximum = {source->bounds.max.x, source->bounds.max.y,
                    source->bounds.max.z, 0.0f},
        .childAndRange = {
            source->left == UINT32_MAX ? UINT32_MAX : source->left + nodeOffset,
            source->right == UINT32_MAX ? UINT32_MAX : source->right + nodeOffset,
            source->first + firstOffset, source->count},
    };
}

static int MeshNodeOffsets(const RayScene *scene, uint32_t *offsets);
static void PackInstance(RayGpuInstance *target, const RayInstance *source,
                         uint32_t meshNodeOffset);

int RayGpuPackScene(const RayScene *scene, const RayGpuSceneLayout *layout,
                    RayGpuNode *nodes, size_t nodeCapacity,
                    RayGpuTriangle *triangles, size_t triangleCapacity,
                    uint32_t *indices, size_t indexCapacity,
                    RayGpuInstance *instances, size_t instanceCapacity) {
    RayGpuSceneLayout expected;
    uint32_t nodeOffset, triangleOffset, indexOffset;
    if (!SceneCounts(scene, &expected) || layout == NULL ||
        memcmp(layout, &expected, sizeof(expected)) != 0 ||
        nodeCapacity < layout->nodeCount ||
        triangleCapacity < layout->triangleCount ||
        indexCapacity < layout->indexCount ||
        instanceCapacity < layout->instanceCount || nodes == NULL ||
        triangles == NULL || indices == NULL || instances == NULL)
        return 0;

    for (uint32_t i = 0; i < scene->nodeCount; ++i)
        PackNode(&nodes[i], &scene->nodes[i], 0, 0);
    memcpy(indices, scene->indices,
           (size_t)scene->instanceCount * sizeof(*indices));
    nodeOffset = scene->nodeCount;
    triangleOffset = 0;
    indexOffset = scene->instanceCount;

    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        const RayInstance *source = &scene->instances[i];
        const RayMesh *mesh = source->mesh;
        uint32_t meshNodeOffset = 0;
        int first = 1;
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (scene->instances[prior].mesh == mesh) {
                meshNodeOffset = instances[prior].meshAndFlags[0];
                first = 0;
                break;
            }
        }
        if (first) {
            meshNodeOffset = nodeOffset;
            for (uint32_t n = 0; n < mesh->nodeCount; ++n)
                PackNode(&nodes[nodeOffset + n], &mesh->nodes[n], nodeOffset,
                         indexOffset);
            for (uint32_t t = 0; t < mesh->triangleCount; ++t) {
                const RayTriangle *triangle = &mesh->triangles[t];
                RayGpuTriangle *gpu = &triangles[triangleOffset + t];
                gpu->vertex0[0] = triangle->vertex[0].x;
                gpu->vertex0[1] = triangle->vertex[0].y;
                gpu->vertex0[2] = triangle->vertex[0].z;
                gpu->vertex0[3] = 0.0f;
                gpu->vertex1[0] = triangle->vertex[1].x;
                gpu->vertex1[1] = triangle->vertex[1].y;
                gpu->vertex1[2] = triangle->vertex[1].z;
                gpu->vertex1[3] = 0.0f;
                gpu->vertex2[0] = triangle->vertex[2].x;
                gpu->vertex2[1] = triangle->vertex[2].y;
                gpu->vertex2[2] = triangle->vertex[2].z;
                gpu->vertex2[3] = 0.0f;
            }
            for (uint32_t k = 0; k < mesh->triangleCount; ++k)
                indices[indexOffset + k] = mesh->indices[k] + triangleOffset;
            nodeOffset += mesh->nodeCount;
            triangleOffset += mesh->triangleCount;
            indexOffset += mesh->triangleCount;
        }
        PackInstance(&instances[i], source, meshNodeOffset);
    }
    return nodeOffset == layout->nodeCount &&
           triangleOffset == layout->triangleCount &&
           indexOffset == layout->indexCount;
}

/* Node offset of every instance's BLAS in the packed node array, in one
 * pass: unique meshes are laid out in first-appearance order after the TLAS.
 * The previous per-instance recursion was cubic in the instance count and
 * cost several milliseconds a frame on a full race grid. */
static int MeshNodeOffsets(const RayScene *scene, uint32_t *offsets) {
    const RayMesh **unique;
    uint32_t *uniqueOffsets;
    uint32_t uniqueCount = 0;
    uint32_t running = scene->nodeCount;

    if (scene->instanceCount == 0) return 1;
    unique = malloc((size_t)scene->instanceCount * sizeof(*unique));
    uniqueOffsets = malloc((size_t)scene->instanceCount * sizeof(*uniqueOffsets));
    if (unique == NULL || uniqueOffsets == NULL) {
        free(unique);
        free(uniqueOffsets);
        return 0;
    }
    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        const RayMesh *mesh = scene->instances[i].mesh;
        uint32_t u;
        for (u = 0; u < uniqueCount; ++u)
            if (unique[u] == mesh) break;
        if (u == uniqueCount) {
            unique[u] = mesh;
            uniqueOffsets[u] = running;
            running += mesh->nodeCount;
            ++uniqueCount;
        }
        offsets[i] = uniqueOffsets[u];
    }
    free(unique);
    free(uniqueOffsets);
    return 1;
}

static void PackInstance(RayGpuInstance *target, const RayInstance *source,
                         uint32_t meshNodeOffset) {
    for (uint32_t row = 0; row < 3; ++row) {
        for (uint32_t column = 0; column < 3; ++column)
            target->worldToLocal[row][column] =
                source->worldToLocal[row][column];
        target->worldToLocal[row][3] =
            -(source->worldToLocal[row][0] * source->position.x +
              source->worldToLocal[row][1] * source->position.y +
              source->worldToLocal[row][2] * source->position.z);
    }
    target->meshAndFlags[0] = meshNodeOffset;
    target->meshAndFlags[1] = source->mesh->nodeCount;
    target->meshAndFlags[2] = source->flags;
    target->meshAndFlags[3] = 0;
}

int RayGpuPackSceneDynamic(const RayScene *scene,
                           const RayGpuSceneLayout *layout,
                           RayGpuNode *tlasNodes, size_t nodeCapacity,
                           uint32_t *tlasIndices, size_t indexCapacity,
                           RayGpuInstance *instances, size_t instanceCapacity) {
    RayGpuSceneLayout expected;
    if (!SceneCounts(scene, &expected) || layout == NULL ||
        memcmp(layout, &expected, sizeof(expected)) != 0 ||
        nodeCapacity < scene->nodeCount ||
        indexCapacity < scene->instanceCount ||
        instanceCapacity < scene->instanceCount || tlasNodes == NULL ||
        tlasIndices == NULL || instances == NULL)
        return 0;
    for (uint32_t i = 0; i < scene->nodeCount; ++i)
        PackNode(&tlasNodes[i], &scene->nodes[i], 0, 0);
    memcpy(tlasIndices, scene->indices,
           (size_t)scene->instanceCount * sizeof(*tlasIndices));
    {
        uint32_t *offsets = scene->instanceCount != 0
            ? malloc((size_t)scene->instanceCount * sizeof(*offsets)) : NULL;
        if (scene->instanceCount != 0 &&
            (offsets == NULL || !MeshNodeOffsets(scene, offsets))) {
            free(offsets);
            return 0;
        }
        for (uint32_t i = 0; i < scene->instanceCount; ++i)
            PackInstance(&instances[i], &scene->instances[i], offsets[i]);
        free(offsets);
    }
    return 1;
}
