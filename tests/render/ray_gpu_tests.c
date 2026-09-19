#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/ray/ray_gpu.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

static RenderTransform IdentityTransform(void) {
    RenderTransform transform = {0};
    transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    return transform;
}

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

static void TestGpuScenePackingDeduplicatesMeshes(void) {
    RayTriangle source = {
        .vertex = {{-1.0f, -1.0f, 0.0f},
                   {1.0f, -1.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f}},
    };
    RayMesh mesh = {0};
    RayInstance sourceInstances[2];
    RayScene scene = {0};
    RayGpuSceneLayout layout = {0};
    RayGpuNode *nodes = NULL;
    RayGpuTriangle *triangles = NULL;
    RayGpuInstance *instances = NULL;
    uint32_t *indices = NULL;
    RenderTransform first = IdentityTransform();
    RenderTransform second = IdentityTransform();

    first.position = (Vec3){2.0f, 3.0f, 4.0f};
    second.position = (Vec3){8.0f, 0.0f, 0.0f};
    CHECK(RayMeshBuild(&mesh, &source, 1));
    CHECK(RayInstancePrepare(&sourceInstances[0], &mesh, &first, 1, 0));
    CHECK(RayInstancePrepare(&sourceInstances[1], &mesh, &second, 2,
                             RAY_INSTANCE_NO_SHADOW));
    CHECK(RaySceneBuild(&scene, sourceInstances, 2));
    CHECK(RayGpuLayoutForScene(&scene, &layout));
    CHECK(layout.nodeCount == scene.nodeCount + mesh.nodeCount);
    CHECK(layout.triangleCount == mesh.triangleCount);
    CHECK(layout.indexCount == scene.instanceCount + mesh.triangleCount);
    CHECK(layout.instanceCount == 2);
    nodes = malloc(layout.nodeBytes);
    triangles = malloc(layout.triangleBytes);
    indices = malloc(layout.indexBytes);
    instances = malloc(layout.instanceBytes);
    CHECK(nodes != NULL && triangles != NULL && indices != NULL &&
          instances != NULL);
    if (nodes != NULL && triangles != NULL && indices != NULL &&
        instances != NULL) {
        CHECK(RayGpuPackScene(&scene, &layout, nodes, layout.nodeCount,
                              triangles, layout.triangleCount,
                              indices, layout.indexCount,
                              instances, layout.instanceCount));
        CHECK(instances[0].meshAndFlags[0] == scene.nodeCount);
        CHECK(instances[1].meshAndFlags[0] == scene.nodeCount);
        CHECK(instances[1].meshAndFlags[2] == RAY_INSTANCE_NO_SHADOW);
        CHECK(instances[0].worldToLocal[0][3] == -2.0f);
        CHECK(instances[0].worldToLocal[1][3] == -3.0f);
        CHECK(instances[0].worldToLocal[2][3] == -4.0f);
        CHECK(indices[scene.instanceCount] == 0);
        CHECK(nodes[scene.nodeCount].childAndRange[2] == scene.instanceCount);
        memset(nodes, 0xCD, layout.nodeBytes);
        memset(indices, 0xCD, layout.indexBytes);
        memset(instances, 0xCD, layout.instanceBytes);
        second.position.x = 12.0f;
        CHECK(RayInstancePrepare(&sourceInstances[1], &mesh, &second, 2, 0));
        CHECK(RaySceneBuild(&scene, sourceInstances, 2));
        CHECK(RayGpuPackSceneDynamic(
            &scene, &layout, nodes, scene.nodeCount,
            indices, scene.instanceCount, instances, scene.instanceCount));
        CHECK(instances[0].meshAndFlags[0] == scene.nodeCount);
        CHECK(instances[1].meshAndFlags[0] == scene.nodeCount);
        CHECK(instances[1].worldToLocal[0][3] == -12.0f);
        CHECK(((unsigned char *)&nodes[scene.nodeCount])[0] == 0xCD);
        CHECK(((unsigned char *)&indices[scene.instanceCount])[0] == 0xCD);
    }
    free(instances);
    free(indices);
    free(triangles);
    free(nodes);
    RaySceneRelease(&scene);
    RayMeshRelease(&mesh);
}

int main(void) {
    TestGpuPacking();
    TestGpuScenePackingDeduplicatesMeshes();
    if (failures != 0) return 1;
    puts("ray GPU packing tests passed");
    return 0;
}
