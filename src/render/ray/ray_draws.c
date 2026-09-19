#include "ray_draws.h"

#include <stdint.h>
#include <stdlib.h>

int RayMeshBuildDraws(RayMesh *mesh, const RageNativeGpuVertex *vertices,
                      size_t vertexCount, const RageNativeDrawSpan *spans,
                      size_t spanCount, RayDrawInclude include, void *context) {
    RayTriangle *triangles;
    size_t triangleCount = 0;
    size_t written = 0;

    if (mesh == NULL || vertices == NULL || spans == NULL) return 0;
    for (size_t spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
        const RageNativeDrawSpan *span = &spans[spanIndex];
        if (span->vertexCount % 3 != 0 || span->firstVertex > vertexCount ||
            span->vertexCount > vertexCount - span->firstVertex) return 0;
        if (include != NULL && !include(context, span)) continue;
        if ((size_t)span->vertexCount / 3 > SIZE_MAX - triangleCount) return 0;
        triangleCount += span->vertexCount / 3;
    }
    if (triangleCount == 0 ||
        triangleCount > SIZE_MAX / sizeof(*triangles)) return 0;
    triangles = malloc(triangleCount * sizeof(*triangles));
    if (triangles == NULL) return 0;
    for (size_t spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
        const RageNativeDrawSpan *span = &spans[spanIndex];
        if (include != NULL && !include(context, span)) continue;
        for (uint32_t offset = 0; offset < span->vertexCount; offset += 3) {
            RayTriangle *triangle = &triangles[written++];
            for (unsigned corner = 0; corner < 3; ++corner) {
                const float *position =
                    vertices[span->firstVertex + offset + corner].position;
                triangle->vertex[corner] =
                    (Vec3){position[0], position[1], position[2]};
            }
            triangle->material = span->material;
        }
    }
    int built = RayMeshBuild(mesh, triangles, triangleCount);
    free(triangles);
    return built;
}
