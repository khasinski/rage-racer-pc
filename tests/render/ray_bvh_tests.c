#include <math.h>
#include <stdio.h>

#include "render/ray/ray_bvh.h"

static int failures;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++;                                                           \
    }                                                                          \
} while (0)

static RayTriangle Triangle(float z, uint32_t material) {
    return (RayTriangle){
        .vertex = {{-1.0f, -1.0f, z}, {1.0f, -1.0f, z}, {0.0f, 1.0f, z}},
        .material = material,
    };
}

static Ray ForwardRay(void) {
    return (Ray){
        .origin = {0.0f, 0.0f, 0.0f},
        .direction = {0.0f, 0.0f, 1.0f},
        .minDistance = 0.001f,
        .maxDistance = 100.0f,
    };
}

static void TestRayValidationAndBounds(void) {
    Ray ray = ForwardRay();
    RayBounds bounds = {{-1.0f, -1.0f, 4.0f}, {1.0f, 1.0f, 6.0f}};
    float nearDistance = 0.0f;

    CHECK(RayValid(&ray));
    CHECK(RayIntersectBounds(&ray, &bounds, ray.maxDistance, &nearDistance));
    CHECK(fabsf(nearDistance - 4.0f) < 0.0001f);
    ray.origin.x = 2.0f;
    CHECK(!RayIntersectBounds(&ray, &bounds, ray.maxDistance, NULL));
    ray = ForwardRay();
    ray.direction = (Vec3){0};
    CHECK(!RayValid(&ray));
    ray = ForwardRay();
    ray.maxDistance = ray.minDistance - 1.0f;
    CHECK(!RayValid(&ray));
}

static void TestTriangleIntersection(void) {
    Ray ray = ForwardRay();
    RayTriangle triangle = Triangle(5.0f, 17);
    RayHit hit = {0};

    CHECK(RayIntersectTriangle(&ray, &triangle, 0, &hit));
    CHECK(fabsf(hit.distance - 5.0f) < 0.0001f);
    CHECK(fabsf(hit.barycentricU - 0.25f) < 0.0001f);
    CHECK(fabsf(hit.barycentricV - 0.5f) < 0.0001f);
    CHECK(hit.material == 17);
    CHECK(fabsf(hit.normal.z - 1.0f) < 0.0001f);
    CHECK(!RayIntersectTriangle(&ray, &triangle,
                                RAY_TRACE_CULL_BACKFACES, &hit));
    {
        Vec3 temporary = triangle.vertex[1];
        triangle.vertex[1] = triangle.vertex[2];
        triangle.vertex[2] = temporary;
    }
    CHECK(RayIntersectTriangle(&ray, &triangle, 0, &hit));
    CHECK(RayIntersectTriangle(&ray, &triangle,
                               RAY_TRACE_CULL_BACKFACES, &hit));
    triangle.vertex[2] = triangle.vertex[1];
    CHECK(!RayIntersectTriangle(&ray, &triangle, 0, &hit));
}

static void TestBvhFindsClosestTriangle(void) {
    RayTriangle triangles[9];
    RayMesh mesh = {0};
    Ray ray = ForwardRay();
    RayHit hit = {0};

    for (uint32_t index = 0; index < 9; ++index) {
        triangles[index] = Triangle(20.0f - (float)index, 100 + index);
        triangles[index].vertex[0].x += (float)(index % 3) * 0.05f;
    }
    CHECK(RayMeshBuild(&mesh, triangles, 9));
    CHECK(mesh.nodeCount > 1 && mesh.triangleCount == 9);
    CHECK(RayMeshTraceClosest(&mesh, &ray, 0, &hit));
    CHECK(fabsf(hit.distance - 12.0f) < 0.0001f);
    CHECK(hit.triangle == 8 && hit.material == 108);
    CHECK(RayMeshTraceAny(&mesh, &ray, 0));
    ray.maxDistance = 11.0f;
    CHECK(!RayMeshTraceAny(&mesh, &ray, 0));
    RayMeshRelease(&mesh);
    CHECK(mesh.nodes == NULL && mesh.nodeCount == 0);
}

static void TestFailedBuildPreservesMesh(void) {
    RayTriangle triangle = Triangle(3.0f, 7);
    RayMesh mesh = {0};
    Ray ray = ForwardRay();
    RayHit hit = {0};

    CHECK(RayMeshBuild(&mesh, &triangle, 1));
    triangle.vertex[0].x = NAN;
    CHECK(!RayMeshBuild(&mesh, &triangle, 1));
    CHECK(RayMeshTraceClosest(&mesh, &ray, 0, &hit));
    CHECK(hit.material == 7);
    CHECK(!RayMeshBuild(NULL, &triangle, 1));
    CHECK(!RayMeshBuild(&mesh, NULL, 1));
    CHECK(!RayMeshBuild(&mesh, &triangle, 0));
    RayMeshRelease(&mesh);
}

int main(void) {
    TestRayValidationAndBounds();
    TestTriangleIntersection();
    TestBvhFindsClosestTriangle();
    TestFailedBuildPreservesMesh();
    if (failures != 0) return 1;
    puts("ray BVH tests passed");
    return 0;
}
