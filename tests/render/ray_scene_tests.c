#include <math.h>
#include <stdio.h>

#include "render/ray/ray_scene.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                         \
} while (0)

static RayTriangle UnitTriangle(void) {
    return (RayTriangle){
        .vertex = {{-1.0f, -1.0f, 0.0f},
                   {1.0f, -1.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f}},
        .material = 12,
    };
}

static RenderTransform IdentityTransform(void) {
    RenderTransform transform = {0};
    transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    return transform;
}

static void TestTranslatedAndScaledInstance(void) {
    RayTriangle triangle = UnitTriangle();
    RayMesh mesh = {0};
    RayInstance instance = {0};
    RenderTransform transform = IdentityTransform();
    Ray ray = {{3.0f, 2.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 20.0f};
    RayHit hit = {0};

    transform.position = (Vec3){3.0f, 2.0f, 5.0f};
    transform.scale = (Vec3){2.0f, 0.5f, 3.0f};
    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RayInstancePrepare(&instance, &mesh, &transform, 77, 0));
    CHECK(RayInstanceTraceClosest(&instance, &ray, &hit));
    CHECK(fabsf(hit.distance - 5.0f) < 0.0001f);
    CHECK(hit.entity == 77 && hit.material == 12);
    CHECK(fabsf(hit.normal.z - 1.0f) < 0.0001f);
    ray.origin.x = 6.0f;
    CHECK(!RayInstanceTraceAny(&instance, &ray));
    RayMeshRelease(&mesh);
}

static void TestEulerRotationAndNormal(void) {
    RayTriangle triangle = UnitTriangle();
    RayMesh mesh = {0};
    RayInstance instance = {0};
    RenderTransform transform = IdentityTransform();
    Ray ray = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.01f, 20.0f};
    RayHit hit = {0};

    transform.position.x = 4.0f;
    transform.rotation.y = 90.0f;
    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RayInstancePrepare(&instance, &mesh, &transform, 4, 0));
    CHECK(RayInstanceTraceClosest(&instance, &ray, &hit));
    CHECK(fabsf(hit.distance - 4.0f) < 0.0001f);
    CHECK(fabsf(hit.normal.x - 1.0f) < 0.0001f);
    RayMeshRelease(&mesh);
}

static void TestQuaternionAndInvalidTransform(void) {
    RayTriangle triangle = UnitTriangle();
    RayMesh mesh = {0};
    RayInstance instance = {0};
    RenderTransform transform = IdentityTransform();
    const float halfTurn = 0.70710678118f;
    Ray ray = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.01f, 20.0f};
    RayHit hit = {0};

    transform.position.x = 6.0f;
    transform.orientation = (Quaternion){0.0f, halfTurn, 0.0f, halfTurn};
    transform.hasOrientation = 1;
    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RayInstancePrepare(&instance, &mesh, &transform, 6, 0));
    CHECK(RayInstanceTraceClosest(&instance, &ray, &hit));
    CHECK(fabsf(hit.distance - 6.0f) < 0.0001f);
    transform.scale.y = 0.0f;
    CHECK(!RayInstancePrepare(&instance, &mesh, &transform, 6, 0));
    transform = IdentityTransform();
    transform.position.x = NAN;
    CHECK(!RayInstancePrepare(&instance, &mesh, &transform, 6, 0));
    RayMeshRelease(&mesh);
}

static void TestReflectedInstanceNormal(void) {
    RayTriangle triangle = UnitTriangle();
    RayMesh mesh = {0};
    RayInstance instance = {0};
    RenderTransform transform = IdentityTransform();
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 20.0f};
    RayHit hit = {0};

    transform.position.z = 3.0f;
    transform.scale.x = -1.0f;
    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RayInstancePrepare(&instance, &mesh, &transform, 8,
                             RAY_INSTANCE_CULL_BACKFACES));
    CHECK(RayInstanceTraceClosest(&instance, &ray, &hit));
    CHECK(fabsf(hit.normal.z + 1.0f) < 0.0001f);
    RayMeshRelease(&mesh);
}

static void TestSceneTlasFindsClosestInstance(void) {
    RayTriangle triangle = UnitTriangle();
    RayMesh mesh = {0};
    RayInstance instances[9];
    RayScene scene = {0};
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 100.0f};
    RayHit hit = {0};

    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    for (uint32_t index = 0; index < 9; ++index) {
        RenderTransform transform = IdentityTransform();
        transform.position.z = 20.0f - (float)index;
        transform.position.x = (index % 2) ? 8.0f : 0.0f;
        CHECK(RayInstancePrepare(&instances[index], &mesh, &transform,
                                 100 + index, 0));
    }
    CHECK(RaySceneBuild(&scene, instances, 9));
    CHECK(scene.nodeCount > 1 && scene.instanceCount == 9);
    CHECK(RaySceneTraceClosest(&scene, &ray, &hit));
    CHECK(fabsf(hit.distance - 12.0f) < 0.0001f);
    CHECK(hit.entity == 108 && hit.instance == 8);
    CHECK(RaySceneTraceAny(&scene, &ray));
    ray.maxDistance = 11.0f;
    CHECK(!RaySceneTraceAny(&scene, &ray));
    CHECK(!RaySceneBuild(&scene, NULL, 1));
    CHECK(RaySceneTraceAny(&scene, &(Ray){
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.01f, 100.0f}));
    RaySceneRelease(&scene);
    RayMeshRelease(&mesh);
}

int main(void) {
    TestTranslatedAndScaledInstance();
    TestEulerRotationAndNormal();
    TestQuaternionAndInvalidTransform();
    TestReflectedInstanceNormal();
    TestSceneTlasFindsClosestInstance();
    if (failures != 0) return 1;
    puts("ray scene tests passed");
    return 0;
}
