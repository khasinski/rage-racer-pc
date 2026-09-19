#include <stdio.h>

#include "render/ray/ray_world.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

typedef struct MeshLookup {
    const RayMesh *mesh;
    uint32_t excludedMesh;
} MeshLookup;

static const RayMesh *Lookup(void *context,
                             const RenderMeshInstance *instance) {
    const MeshLookup *lookup = context;
    return instance->mesh == lookup->excludedMesh ? NULL : lookup->mesh;
}

static RenderMeshInstance Instance(uint32_t entity, uint32_t mesh, float z,
                                   RenderPass pass) {
    RenderMeshInstance instance = {0};
    instance.entity = entity;
    instance.mesh = mesh;
    instance.pass = pass;
    instance.transform.position.z = z;
    instance.transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    return instance;
}

static void TestWorldConversion(void) {
    RayTriangle triangle = {
        .vertex = {{-1.0f, -1.0f, 0.0f},
                   {1.0f, -1.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f}},
        .material = 3,
    };
    RayMesh mesh = {0};
    MeshLookup lookup = {&mesh, 99};
    RenderMeshInstance renderInstances[4] = {
        Instance(10, 1, 8.0f, RAGE_RENDER_PASS_MAIN),
        Instance(11, 2, 4.0f, RAGE_RENDER_PASS_MAIN),
        Instance(12, 99, 2.0f, RAGE_RENDER_PASS_MAIN),
        Instance(13, 3, 1.0f, RAGE_RENDER_PASS_MIRROR),
    };
    renderInstances[0].flags = RAGE_RENDER_INSTANCE_RAY_ONLY;
    renderInstances[1].flags = RAGE_RENDER_INSTANCE_RAY_NO_SHADOW;
    RenderWorld world = {0};
    RayScene scene = {0};
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 20.0f};
    RayHit hit = {0};

    world.instances = renderInstances;
    world.instanceCapacity = 4;
    world.instanceCount = 4;
    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RaySceneBuildWorld(&scene, &world, RAGE_RENDER_PASS_MAIN,
                             Lookup, &lookup));
    CHECK(scene.instanceCount == 2);
    CHECK((scene.instances[1].flags & RAY_INSTANCE_NO_SHADOW) != 0);
    CHECK(RaySceneTraceClosest(&scene, &ray, &hit));
    CHECK(hit.entity == 11 && hit.distance == 4.0f);
    ray.maxDistance = 6.0f;
    CHECK(!RaySceneTraceAny(&scene, &ray));
    ray.maxDistance = 20.0f;
    CHECK(RaySceneBuildWorld(&scene, &world, RAGE_RENDER_PASS_MIRROR,
                             Lookup, &lookup));
    CHECK(scene.instanceCount == 1);
    CHECK(RaySceneTraceClosest(&scene, &ray, &hit));
    CHECK(hit.entity == 13 && hit.distance == 1.0f);
    lookup.excludedMesh = 3;
    CHECK(RaySceneBuildWorld(&scene, &world, RAGE_RENDER_PASS_MIRROR,
                             Lookup, &lookup));
    CHECK(scene.instanceCount == 0);
    CHECK(!RaySceneTraceAny(&scene, &ray));
    RaySceneRelease(&scene);
    RayMeshRelease(&mesh);
}

int main(void) {
    TestWorldConversion();
    if (failures != 0) return 1;
    puts("ray world tests passed");
    return 0;
}
