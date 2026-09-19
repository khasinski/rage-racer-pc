#include <stdio.h>

#include "render/ray/ray_draws.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

static int OpaqueOnly(void *context, const RageNativeDrawSpan *span) {
    uint32_t excludedMaterial = *(const uint32_t *)context;
    return span->material != excludedMaterial;
}

static void TestDrawFlatteningAndFilter(void) {
    RageNativeGpuVertex vertices[6] = {0};
    RageNativeDrawSpan spans[2] = {0};
    RayMesh mesh = {0};
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 20.0f};
    RayHit hit = {0};
    uint32_t excluded = 8;
    const float positions[6][3] = {
        {-1, -1, 3}, {1, -1, 3}, {0, 1, 3},
        {-1, -1, 6}, {1, -1, 6}, {0, 1, 6},
    };

    for (unsigned index = 0; index < 6; ++index)
        for (unsigned axis = 0; axis < 3; ++axis)
            vertices[index].position[axis] = positions[index][axis];
    spans[0].firstVertex = 0;
    spans[0].vertexCount = 3;
    spans[0].material = excluded;
    spans[1].firstVertex = 3;
    spans[1].vertexCount = 3;
    spans[1].material = 9;
    CHECK(RayMeshBuildDraws(&mesh, vertices, 6, spans, 2,
                            OpaqueOnly, &excluded));
    CHECK(mesh.triangleCount == 1);
    CHECK(RayMeshTraceClosest(&mesh, &ray, 0, &hit));
    CHECK(hit.distance == 6.0f && hit.material == 9);
    RayMeshRelease(&mesh);
    spans[1].firstVertex = 5;
    CHECK(!RayMeshBuildDraws(&mesh, vertices, 6, spans, 2, NULL, NULL));
}

int main(void) {
    TestDrawFlatteningAndFilter();
    if (failures != 0) return 1;
    puts("ray draw tests passed");
    return 0;
}
