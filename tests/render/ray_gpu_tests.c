#include <stdio.h>
#include <stdlib.h>

#include "render/ray/ray_gpu.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

static void TestGpuPacking(void) {
    RayTriangle source[6];
    RayMesh mesh = {0};
    RayGpuLayout layout = {0};
    RayGpuNode *nodes;
    RayGpuTriangle *triangles;
    uint32_t *indices;

    for (uint32_t index = 0; index < 6; ++index) {
        float z = 2.0f + (float)index;
        source[index] = (RayTriangle){
            .vertex = {{-1.0f, -1.0f, z},
                       {1.0f, -1.0f, z},
                       {0.0f, 1.0f, z}},
            .material = index,
        };
    }
    CHECK(RayMeshBuild(&mesh, source, 6));
    CHECK(RayGpuLayoutForMesh(&mesh, &layout));
    CHECK(layout.nodeBytes == mesh.nodeCount * sizeof(RayGpuNode));
    CHECK(layout.triangleBytes == 6 * sizeof(RayGpuTriangle));
    CHECK(layout.indexBytes == 6 * sizeof(uint32_t));
    nodes = malloc(layout.nodeBytes);
    triangles = malloc(layout.triangleBytes);
    indices = malloc(layout.indexBytes);
    CHECK(nodes != NULL && triangles != NULL && indices != NULL);
    if (nodes != NULL && triangles != NULL && indices != NULL) {
        CHECK(RayGpuPackMesh(&mesh, nodes, mesh.nodeCount,
                             triangles, mesh.triangleCount,
                             indices, mesh.triangleCount));
        CHECK(nodes[0].minimum[2] == 2.0f);
        CHECK(nodes[0].maximum[2] == 7.0f);
        CHECK(nodes[0].childAndRange[3] == 0);
        CHECK(triangles[5].vertex0[2] == 7.0f);
        CHECK(triangles[5].vertex0[3] == 0.0f);
        CHECK(!RayGpuPackMesh(&mesh, nodes, 0, triangles, 6, indices, 6));
    }
    free(indices);
    free(triangles);
    free(nodes);
    RayMeshRelease(&mesh);
}

int main(void) {
    TestGpuPacking();
    if (failures != 0) return 1;
    puts("ray GPU packing tests passed");
    return 0;
}
