#include "ray_bvh.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    RAY_BVH_LEAF_TRIANGLES = 4,
};

#define RAY_BVH_NO_NODE UINT32_MAX

typedef struct RayBvhBuild {
    RayTriangle *triangles;
    uint32_t *indices;
    RayBvhNode *nodes;
    uint32_t nodeCount;
    uint32_t nodeCapacity;
} RayBvhBuild;

static float Component(Vec3 value, unsigned axis) {
    if (axis == 0) return value.x;
    if (axis == 1) return value.y;
    return value.z;
}

static Vec3 Minimum(Vec3 left, Vec3 right) {
    return (Vec3){fminf(left.x, right.x), fminf(left.y, right.y),
                  fminf(left.z, right.z)};
}

static Vec3 Maximum(Vec3 left, Vec3 right) {
    return (Vec3){fmaxf(left.x, right.x), fmaxf(left.y, right.y),
                  fmaxf(left.z, right.z)};
}

static RayBounds TriangleBounds(const RayTriangle *triangle) {
    RayBounds bounds = {triangle->vertex[0], triangle->vertex[0]};

    bounds.min = Minimum(bounds.min, triangle->vertex[1]);
    bounds.min = Minimum(bounds.min, triangle->vertex[2]);
    bounds.max = Maximum(bounds.max, triangle->vertex[1]);
    bounds.max = Maximum(bounds.max, triangle->vertex[2]);
    return bounds;
}

static Vec3 TriangleCenter(const RayTriangle *triangle) {
    return (Vec3){
        (triangle->vertex[0].x + triangle->vertex[1].x +
         triangle->vertex[2].x) / 3.0f,
        (triangle->vertex[0].y + triangle->vertex[1].y +
         triangle->vertex[2].y) / 3.0f,
        (triangle->vertex[0].z + triangle->vertex[1].z +
         triangle->vertex[2].z) / 3.0f,
    };
}

static RayBounds RangeBounds(const RayBvhBuild *build, uint32_t first,
                             uint32_t count) {
    RayBounds bounds = TriangleBounds(
        &build->triangles[build->indices[first]]);

    for (uint32_t offset = 1; offset < count; ++offset) {
        RayBounds triangle = TriangleBounds(
            &build->triangles[build->indices[first + offset]]);
        bounds.min = Minimum(bounds.min, triangle.min);
        bounds.max = Maximum(bounds.max, triangle.max);
    }
    return bounds;
}

static RayBounds CenterBounds(const RayBvhBuild *build, uint32_t first,
                              uint32_t count) {
    Vec3 center = TriangleCenter(
        &build->triangles[build->indices[first]]);
    RayBounds bounds = {center, center};

    for (uint32_t offset = 1; offset < count; ++offset) {
        center = TriangleCenter(
            &build->triangles[build->indices[first + offset]]);
        bounds.min = Minimum(bounds.min, center);
        bounds.max = Maximum(bounds.max, center);
    }
    return bounds;
}

static unsigned LargestAxis(const RayBounds *bounds) {
    Vec3 extent = {bounds->max.x - bounds->min.x,
                   bounds->max.y - bounds->min.y,
                   bounds->max.z - bounds->min.z};

    if (extent.y > extent.x && extent.y >= extent.z) return 1;
    return extent.z > extent.x ? 2 : 0;
}

static uint32_t Partition(RayBvhBuild *build, uint32_t first,
                          uint32_t count, unsigned axis, float split) {
    uint32_t low = first;
    uint32_t high = first + count;

    while (low < high) {
        Vec3 center = TriangleCenter(&build->triangles[build->indices[low]]);
        if (Component(center, axis) < split) {
            low++;
        } else {
            uint32_t temporary;
            high--;
            temporary = build->indices[low];
            build->indices[low] = build->indices[high];
            build->indices[high] = temporary;
        }
    }
    if (low == first || low == first + count) return first + count / 2;
    return low;
}

static uint32_t BuildNode(RayBvhBuild *build, uint32_t first,
                          uint32_t count) {
    uint32_t nodeIndex;
    RayBvhNode *node;

    if (build->nodeCount >= build->nodeCapacity || count == 0) {
        return RAY_BVH_NO_NODE;
    }
    nodeIndex = build->nodeCount++;
    node = &build->nodes[nodeIndex];
    node->bounds = RangeBounds(build, first, count);
    node->first = first;
    node->count = count;
    node->left = RAY_BVH_NO_NODE;
    node->right = RAY_BVH_NO_NODE;
    if (count > RAY_BVH_LEAF_TRIANGLES) {
        RayBounds centers = CenterBounds(build, first, count);
        unsigned axis = LargestAxis(&centers);
        float split = (Component(centers.min, axis) +
                       Component(centers.max, axis)) * 0.5f;
        uint32_t middle = Partition(build, first, count, axis, split);
        uint32_t left = BuildNode(build, first, middle - first);
        uint32_t right = BuildNode(build, middle, first + count - middle);

        if (left == RAY_BVH_NO_NODE || right == RAY_BVH_NO_NODE) {
            return RAY_BVH_NO_NODE;
        }
        node = &build->nodes[nodeIndex];
        node->first = 0;
        node->count = 0;
        node->left = left;
        node->right = right;
    }
    return nodeIndex;
}

int RayMeshBuild(RayMesh *mesh, const RayTriangle *triangles, size_t count) {
    RayMesh next = {0};
    RayBvhBuild build;

    if (mesh == NULL || triangles == NULL || count == 0 ||
        count > UINT32_MAX / 2) {
        return 0;
    }
    next.triangles = malloc(count * sizeof(*next.triangles));
    next.indices = malloc(count * sizeof(*next.indices));
    next.nodes = calloc(count * 2, sizeof(*next.nodes));
    if (next.triangles == NULL || next.indices == NULL || next.nodes == NULL) {
        RayMeshRelease(&next);
        return 0;
    }
    memcpy(next.triangles, triangles, count * sizeof(*next.triangles));
    for (size_t index = 0; index < count; ++index) {
        const RayTriangle *triangle = &next.triangles[index];
        for (unsigned vertex = 0; vertex < 3; ++vertex) {
            Vec3 value = triangle->vertex[vertex];
            if (!isfinite(value.x) || !isfinite(value.y) ||
                !isfinite(value.z)) {
                RayMeshRelease(&next);
                return 0;
            }
        }
        next.indices[index] = (uint32_t)index;
    }
    build = (RayBvhBuild){next.triangles, next.indices, next.nodes, 0,
                          (uint32_t)(count * 2)};
    if (BuildNode(&build, 0, (uint32_t)count) != 0) {
        RayMeshRelease(&next);
        return 0;
    }
    next.triangleCount = (uint32_t)count;
    next.nodeCount = build.nodeCount;
    RayMeshRelease(mesh);
    *mesh = next;
    return 1;
}

void RayMeshRelease(RayMesh *mesh) {
    if (mesh == NULL) return;
    free(mesh->triangles);
    free(mesh->indices);
    free(mesh->nodes);
    *mesh = (RayMesh){0};
}

static int Trace(const RayMesh *mesh, const Ray *ray, uint32_t flags,
                 RayHit *hit, int any) {
    uint32_t stack[64];
    uint32_t stackCount = 0;
    float closest;
    int found = 0;

    if (mesh == NULL || ray == NULL || !RayValid(ray) ||
        mesh->nodes == NULL || mesh->indices == NULL ||
        mesh->triangles == NULL || mesh->nodeCount == 0) {
        return 0;
    }
    closest = ray->maxDistance;
    stack[stackCount++] = 0;
    while (stackCount != 0) {
        uint32_t nodeIndex = stack[--stackCount];
        const RayBvhNode *node;

        if (nodeIndex >= mesh->nodeCount) return 0;
        node = &mesh->nodes[nodeIndex];
        if (!RayIntersectBounds(ray, &node->bounds, closest, NULL)) continue;
        if (node->count != 0) {
            if (node->first > mesh->triangleCount ||
                node->count > mesh->triangleCount - node->first) return 0;
            for (uint32_t offset = 0; offset < node->count; ++offset) {
                uint32_t triangleIndex = mesh->indices[node->first + offset];
                RayHit candidate;

                if (triangleIndex >= mesh->triangleCount ||
                    !RayIntersectTriangle(ray, &mesh->triangles[triangleIndex],
                                          flags, &candidate) ||
                    candidate.distance > closest) {
                    continue;
                }
                if (any) return 1;
                candidate.triangle = triangleIndex;
                closest = candidate.distance;
                *hit = candidate;
                found = 1;
            }
        } else {
            if (node->left >= mesh->nodeCount ||
                node->right >= mesh->nodeCount || stackCount + 2 > 64) {
                return 0;
            }
            stack[stackCount++] = node->right;
            stack[stackCount++] = node->left;
        }
    }
    return found;
}

int RayMeshTraceClosest(const RayMesh *mesh, const Ray *ray,
                        uint32_t flags, RayHit *hit) {
    if (hit == NULL) return 0;
    return Trace(mesh, ray, flags, hit, 0);
}

int RayMeshTraceAny(const RayMesh *mesh, const Ray *ray, uint32_t flags) {
    RayHit ignored;
    return Trace(mesh, ray, flags, &ignored, 1);
}

int RayMeshBounds(const RayMesh *mesh, RayBounds *out) {
    if (mesh == NULL || out == NULL || mesh->nodes == NULL ||
        mesh->nodeCount == 0) {
        return 0;
    }
    *out = mesh->nodes[0].bounds;
    return 1;
}
