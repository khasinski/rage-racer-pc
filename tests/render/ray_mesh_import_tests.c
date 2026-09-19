#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/ray/ray_mesh_import.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

static int BuildFixture(uint8_t **bytes, size_t *size, uint32_t material) {
    RageRuntimeMeshLayout layout;
    RageRuntimeVertex vertices[3] = {
        {.position = {-1.0f, -1.0f, 4.0f}, .material = material},
        {.position = {1.0f, -1.0f, 4.0f}, .material = material},
        {.position = {0.0f, 1.0f, 4.0f}, .material = material},
    };
    const uint32_t offsets[2] = {0, 3};
    const uint32_t indices[3] = {0, 1, 2};

    if (!RuntimeMeshLayout(1, 3, 3, &layout)) return 0;
    *bytes = calloc(1, layout.totalSize);
    if (*bytes == NULL) return 0;
    *size = layout.totalSize;
    if (!RuntimeMeshEncodeHeader(*bytes, *size, 1, 3, 3)) return 0;
    memcpy(*bytes + layout.offsetsOffset, offsets, sizeof(offsets));
    for (size_t index = 0; index < 3; ++index) {
        if (!RuntimeVertexEncode(*bytes + layout.verticesOffset +
                                 index * RAGE_RUNTIME_VERTEX_BYTES,
                                 RAGE_RUNTIME_VERTEX_BYTES,
                                 &vertices[index])) return 0;
    }
    memcpy(*bytes + layout.indicesOffset, indices, sizeof(indices));
    return 1;
}

static void TestRuntimeMeshImport(void) {
    uint8_t *bytes = NULL;
    size_t size = 0;
    RageRuntimeMesh source = {0};
    RayMesh mesh = {0};
    RayHit hit = {0};
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.001f, 20.0f};
    uint32_t taggedMaterial = 9 | RAGE_RUNTIME_MATERIAL_SCROLL_U;

    CHECK(BuildFixture(&bytes, &size, taggedMaterial));
    CHECK(RuntimeMeshOpen(&source, bytes, size));
    CHECK(RayMeshBuildRuntime(&mesh, &source, 0));
    CHECK(mesh.triangleCount == 1);
    CHECK(RayMeshTraceClosest(&mesh, &ray, 0, &hit));
    CHECK(hit.material == 9);
    CHECK(!RayMeshBuildRuntime(&mesh, &source, 1));
    CHECK(RayMeshTraceClosest(&mesh, &ray, 0, &hit));
    RayMeshRelease(&mesh);
    free(bytes);
}

static void TestRejectsMixedTriangleMaterial(void) {
    uint8_t *bytes = NULL;
    size_t size = 0;
    RageRuntimeMesh source = {0};
    RageRuntimeVertex vertex;
    RayMesh mesh = {0};

    CHECK(BuildFixture(&bytes, &size, 4));
    CHECK(RuntimeMeshOpen(&source, bytes, size));
    CHECK(RuntimeMeshVertex(&source, 1, &vertex));
    vertex.material = 5;
    CHECK(RuntimeVertexEncode(bytes + source.verticesOffset +
                              RAGE_RUNTIME_VERTEX_BYTES,
                              RAGE_RUNTIME_VERTEX_BYTES, &vertex));
    CHECK(RuntimeMeshOpen(&source, bytes, size));
    CHECK(!RayMeshBuildRuntime(&mesh, &source, 0));
    RayMeshRelease(&mesh);
    free(bytes);
}

int main(void) {
    TestRuntimeMeshImport();
    TestRejectsMixedTriangleMaterial();
    if (failures != 0) return 1;
    puts("ray mesh import tests passed");
    return 0;
}
