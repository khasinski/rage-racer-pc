#include "ray_mesh_import.h"

#include <stdint.h>
#include <stdlib.h>

static uint32_t RayMaterial(uint32_t material) {
    if (material == UINT32_MAX) return UINT32_MAX;
    return material & RAGE_RUNTIME_MATERIAL_INDEX_MASK;
}

int RayMeshBuildRuntime(RayMesh *out, const RageRuntimeMesh *source,
                        uint32_t meshIndex) {
    RayTriangle *triangles;
    uint32_t first;
    uint32_t indexCount;
    size_t triangleCount;
    int built;

    if (out == NULL || source == NULL ||
        !RuntimeMeshRange(source, meshIndex, &first, &indexCount) ||
        indexCount == 0 || indexCount % 3 != 0) {
        return 0;
    }
    triangleCount = indexCount / 3;
    if (triangleCount > SIZE_MAX / sizeof(*triangles)) return 0;
    triangles = malloc(triangleCount * sizeof(*triangles));
    if (triangles == NULL) return 0;

    for (size_t triangleIndex = 0; triangleIndex < triangleCount;
         ++triangleIndex) {
        RayTriangle *triangle = &triangles[triangleIndex];
        uint32_t material = UINT32_MAX;

        for (uint32_t corner = 0; corner < 3; ++corner) {
            uint32_t index;
            RageRuntimeVertex vertex;
            size_t sourceIndex = (size_t)first + triangleIndex * 3 + corner;

            if (sourceIndex > UINT32_MAX ||
                !RuntimeMeshIndex(source, (uint32_t)sourceIndex, &index) ||
                !RuntimeMeshVertex(source, index, &vertex)) {
                free(triangles);
                return 0;
            }
            triangle->vertex[corner] = (Vec3){
                vertex.position[0], vertex.position[1], vertex.position[2]};
            if (corner == 0) {
                material = RayMaterial(vertex.material);
            } else if (RayMaterial(vertex.material) != material) {
                free(triangles);
                return 0;
            }
        }
        triangle->material = material;
    }
    built = RayMeshBuild(out, triangles, triangleCount);
    free(triangles);
    return built;
}
