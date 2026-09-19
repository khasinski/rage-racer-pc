#include "ray_scene.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "render/render_instance_transform.h"

static Vec3 MatrixVector(const float matrix[3][3], Vec3 value) {
    return (Vec3){
        matrix[0][0] * value.x + matrix[0][1] * value.y +
            matrix[0][2] * value.z,
        matrix[1][0] * value.x + matrix[1][1] * value.y +
            matrix[1][2] * value.z,
        matrix[2][0] * value.x + matrix[2][1] * value.y +
            matrix[2][2] * value.z,
    };
}

static int InvertMatrix(const float matrix[3][3], float inverse[3][3],
                        float *orientationSign) {
    double determinant =
        (double)matrix[0][0] *
            ((double)matrix[1][1] * matrix[2][2] -
             (double)matrix[1][2] * matrix[2][1]) -
        (double)matrix[0][1] *
            ((double)matrix[1][0] * matrix[2][2] -
             (double)matrix[1][2] * matrix[2][0]) +
        (double)matrix[0][2] *
            ((double)matrix[1][0] * matrix[2][1] -
             (double)matrix[1][1] * matrix[2][0]);

    if (!isfinite(determinant) || fabs(determinant) < 1.0e-12) return 0;
    *orientationSign = determinant < 0.0 ? -1.0f : 1.0f;
    determinant = 1.0 / determinant;
    inverse[0][0] = (float)(((double)matrix[1][1] * matrix[2][2] -
                             (double)matrix[1][2] * matrix[2][1]) * determinant);
    inverse[0][1] = (float)(((double)matrix[0][2] * matrix[2][1] -
                             (double)matrix[0][1] * matrix[2][2]) * determinant);
    inverse[0][2] = (float)(((double)matrix[0][1] * matrix[1][2] -
                             (double)matrix[0][2] * matrix[1][1]) * determinant);
    inverse[1][0] = (float)(((double)matrix[1][2] * matrix[2][0] -
                             (double)matrix[1][0] * matrix[2][2]) * determinant);
    inverse[1][1] = (float)(((double)matrix[0][0] * matrix[2][2] -
                             (double)matrix[0][2] * matrix[2][0]) * determinant);
    inverse[1][2] = (float)(((double)matrix[0][2] * matrix[1][0] -
                             (double)matrix[0][0] * matrix[1][2]) * determinant);
    inverse[2][0] = (float)(((double)matrix[1][0] * matrix[2][1] -
                             (double)matrix[1][1] * matrix[2][0]) * determinant);
    inverse[2][1] = (float)(((double)matrix[0][1] * matrix[2][0] -
                             (double)matrix[0][0] * matrix[2][1]) * determinant);
    inverse[2][2] = (float)(((double)matrix[0][0] * matrix[1][1] -
                             (double)matrix[0][1] * matrix[1][0]) * determinant);
    return 1;
}

static Vec3 TransformPoint(const RayInstance *instance, Vec3 point) {
    point = MatrixVector(instance->localToWorld, point);
    point.x += instance->position.x;
    point.y += instance->position.y;
    point.z += instance->position.z;
    return point;
}

static Vec3 WorldToLocalPoint(const RayInstance *instance, Vec3 point) {
    point.x -= instance->position.x;
    point.y -= instance->position.y;
    point.z -= instance->position.z;
    return MatrixVector(instance->worldToLocal, point);
}

int RayInstancePrepare(RayInstance *out, const RayMesh *mesh,
                       const RenderTransform *transform, uint32_t entity,
                       uint32_t flags) {
    RenderInstanceTransform prepared;
    RayBounds local;
    RayInstance next = {0};
    Vec3 columns[3];

    if (out == NULL || transform == NULL || !RayMeshBounds(mesh, &local)) {
        return 0;
    }
    prepared = RenderPrepareInstanceTransform(transform);
    columns[0] = RenderRotateInstanceVector(&prepared,
        (Vec3){transform->scale.x, 0.0f, 0.0f});
    columns[1] = RenderRotateInstanceVector(&prepared,
        (Vec3){0.0f, transform->scale.y, 0.0f});
    columns[2] = RenderRotateInstanceVector(&prepared,
        (Vec3){0.0f, 0.0f, transform->scale.z});
    for (unsigned column = 0; column < 3; ++column) {
        next.localToWorld[0][column] = columns[column].x;
        next.localToWorld[1][column] = columns[column].y;
        next.localToWorld[2][column] = columns[column].z;
    }
    if (!InvertMatrix(next.localToWorld, next.worldToLocal,
                      &next.orientationSign) ||
        !isfinite(transform->position.x) || !isfinite(transform->position.y) ||
        !isfinite(transform->position.z)) {
        return 0;
    }
    next.mesh = mesh;
    next.position = transform->position;
    next.entity = entity;
    next.flags = flags;
    for (unsigned corner = 0; corner < 8; ++corner) {
        Vec3 point = {
            (corner & 1) ? local.max.x : local.min.x,
            (corner & 2) ? local.max.y : local.min.y,
            (corner & 4) ? local.max.z : local.min.z,
        };
        point = TransformPoint(&next, point);
        if (corner == 0) {
            next.bounds.min = next.bounds.max = point;
        } else {
            next.bounds.min.x = fminf(next.bounds.min.x, point.x);
            next.bounds.min.y = fminf(next.bounds.min.y, point.y);
            next.bounds.min.z = fminf(next.bounds.min.z, point.z);
            next.bounds.max.x = fmaxf(next.bounds.max.x, point.x);
            next.bounds.max.y = fmaxf(next.bounds.max.y, point.y);
            next.bounds.max.z = fmaxf(next.bounds.max.z, point.z);
        }
    }
    *out = next;
    return 1;
}

static Vec3 TransformNormal(const RayInstance *instance, Vec3 normal) {
    Vec3 world = {
        instance->worldToLocal[0][0] * normal.x +
            instance->worldToLocal[1][0] * normal.y +
            instance->worldToLocal[2][0] * normal.z,
        instance->worldToLocal[0][1] * normal.x +
            instance->worldToLocal[1][1] * normal.y +
            instance->worldToLocal[2][1] * normal.z,
        instance->worldToLocal[0][2] * normal.x +
            instance->worldToLocal[1][2] * normal.y +
            instance->worldToLocal[2][2] * normal.z,
    };
    float length = sqrtf(world.x * world.x + world.y * world.y +
                         world.z * world.z);
    if (length > 0.0f) {
        float inverseLength = instance->orientationSign / length;
        world.x *= inverseLength;
        world.y *= inverseLength;
        world.z *= inverseLength;
    }
    return world;
}

int RayInstanceTraceClosest(const RayInstance *instance, const Ray *worldRay,
                            RayHit *hit) {
    Ray local;
    RayHit candidate;
    uint32_t flags = 0;

    if (instance == NULL || worldRay == NULL || hit == NULL ||
        instance->mesh == NULL ||
        !RayIntersectBounds(worldRay, &instance->bounds,
                            worldRay->maxDistance, NULL)) return 0;
    local = *worldRay;
    local.origin = WorldToLocalPoint(instance, worldRay->origin);
    local.direction = MatrixVector(instance->worldToLocal,
                                   worldRay->direction);
    if (instance->flags & RAY_INSTANCE_CULL_BACKFACES) {
        flags |= RAY_TRACE_CULL_BACKFACES;
        if (instance->orientationSign < 0.0f)
            flags |= RAY_TRACE_REVERSE_WINDING;
    }
    if (!RayMeshTraceClosest(instance->mesh, &local, flags, &candidate)) return 0;
    candidate.normal = TransformNormal(instance, candidate.normal);
    candidate.entity = instance->entity;
    *hit = candidate;
    return 1;
}

int RayInstanceTraceAny(const RayInstance *instance, const Ray *worldRay) {
    RayHit hit;
    return RayInstanceTraceClosest(instance, worldRay, &hit);
}

typedef struct SceneBuild {
    RayInstance *instances;
    uint32_t *indices;
    RayBvhNode *nodes;
    uint32_t nodeCount;
    uint32_t nodeCapacity;
} SceneBuild;

#define RAY_SCENE_NO_NODE UINT32_MAX

enum { RAY_SCENE_LEAF_INSTANCES = 4 };

static float Axis(Vec3 value, unsigned axis) {
    if (axis == 0) return value.x;
    if (axis == 1) return value.y;
    return value.z;
}

static Vec3 BoundsCenter(RayBounds bounds) {
    return (Vec3){(bounds.min.x + bounds.max.x) * 0.5f,
                  (bounds.min.y + bounds.max.y) * 0.5f,
                  (bounds.min.z + bounds.max.z) * 0.5f};
}

static void IncludeBounds(RayBounds *out, RayBounds value) {
    out->min.x = fminf(out->min.x, value.min.x);
    out->min.y = fminf(out->min.y, value.min.y);
    out->min.z = fminf(out->min.z, value.min.z);
    out->max.x = fmaxf(out->max.x, value.max.x);
    out->max.y = fmaxf(out->max.y, value.max.y);
    out->max.z = fmaxf(out->max.z, value.max.z);
}

static RayBounds SceneRangeBounds(const SceneBuild *build, uint32_t first,
                                  uint32_t count) {
    RayBounds bounds = build->instances[build->indices[first]].bounds;
    for (uint32_t offset = 1; offset < count; ++offset)
        IncludeBounds(&bounds,
            build->instances[build->indices[first + offset]].bounds);
    return bounds;
}

static RayBounds SceneCenterBounds(const SceneBuild *build, uint32_t first,
                                   uint32_t count) {
    Vec3 center = BoundsCenter(
        build->instances[build->indices[first]].bounds);
    RayBounds bounds = {center, center};
    for (uint32_t offset = 1; offset < count; ++offset) {
        center = BoundsCenter(
            build->instances[build->indices[first + offset]].bounds);
        RayBounds point = {center, center};
        IncludeBounds(&bounds, point);
    }
    return bounds;
}

static unsigned SceneLargestAxis(RayBounds bounds) {
    Vec3 extent = {bounds.max.x - bounds.min.x,
                   bounds.max.y - bounds.min.y,
                   bounds.max.z - bounds.min.z};
    if (extent.y > extent.x && extent.y >= extent.z) return 1;
    return extent.z > extent.x ? 2 : 0;
}

static uint32_t ScenePartition(SceneBuild *build, uint32_t first,
                               uint32_t count, unsigned axis, float split) {
    uint32_t low = first;
    uint32_t high = first + count;
    while (low < high) {
        Vec3 center = BoundsCenter(build->instances[build->indices[low]].bounds);
        if (Axis(center, axis) < split) {
            ++low;
        } else {
            uint32_t temporary;
            --high;
            temporary = build->indices[low];
            build->indices[low] = build->indices[high];
            build->indices[high] = temporary;
        }
    }
    if (low == first || low == first + count) return first + count / 2;
    return low;
}

static uint32_t BuildSceneNode(SceneBuild *build, uint32_t first,
                               uint32_t count) {
    uint32_t nodeIndex;
    RayBvhNode *node;
    if (count == 0 || build->nodeCount >= build->nodeCapacity)
        return RAY_SCENE_NO_NODE;
    nodeIndex = build->nodeCount++;
    node = &build->nodes[nodeIndex];
    node->bounds = SceneRangeBounds(build, first, count);
    node->first = first;
    node->count = count;
    node->left = node->right = RAY_SCENE_NO_NODE;
    if (count > RAY_SCENE_LEAF_INSTANCES) {
        RayBounds centers = SceneCenterBounds(build, first, count);
        unsigned axis = SceneLargestAxis(centers);
        float split = (Axis(centers.min, axis) + Axis(centers.max, axis)) * 0.5f;
        uint32_t middle = ScenePartition(build, first, count, axis, split);
        uint32_t left = BuildSceneNode(build, first, middle - first);
        uint32_t right = BuildSceneNode(build, middle, first + count - middle);
        if (left == RAY_SCENE_NO_NODE || right == RAY_SCENE_NO_NODE)
            return RAY_SCENE_NO_NODE;
        node = &build->nodes[nodeIndex];
        node->first = node->count = 0;
        node->left = left;
        node->right = right;
    }
    return nodeIndex;
}

int RaySceneBuild(RayScene *scene, const RayInstance *instances, size_t count) {
    RayScene next = {0};
    SceneBuild build;
    if (scene == NULL || instances == NULL || count == 0 ||
        count > UINT32_MAX / 2 || count > SIZE_MAX / sizeof(*next.instances))
        return 0;
    next.instances = malloc(count * sizeof(*next.instances));
    next.indices = malloc(count * sizeof(*next.indices));
    next.nodes = calloc(count * 2, sizeof(*next.nodes));
    if (next.instances == NULL || next.indices == NULL || next.nodes == NULL) {
        RaySceneRelease(&next);
        return 0;
    }
    memcpy(next.instances, instances, count * sizeof(*next.instances));
    for (size_t index = 0; index < count; ++index) {
        if (next.instances[index].mesh == NULL) {
            RaySceneRelease(&next);
            return 0;
        }
        next.indices[index] = (uint32_t)index;
    }
    build = (SceneBuild){next.instances, next.indices, next.nodes, 0,
                         (uint32_t)(count * 2)};
    if (BuildSceneNode(&build, 0, (uint32_t)count) != 0) {
        RaySceneRelease(&next);
        return 0;
    }
    next.instanceCount = (uint32_t)count;
    next.nodeCount = build.nodeCount;
    RaySceneRelease(scene);
    *scene = next;
    return 1;
}

void RaySceneRelease(RayScene *scene) {
    if (scene == NULL) return;
    free(scene->instances);
    free(scene->indices);
    free(scene->nodes);
    *scene = (RayScene){0};
}

static int TraceScene(const RayScene *scene, const Ray *ray, RayHit *hit,
                      int any) {
    uint32_t stack[64];
    uint32_t stackCount = 1;
    float closest;
    int found = 0;
    if (scene == NULL || ray == NULL || hit == NULL || !RayValid(ray) ||
        scene->nodes == NULL || scene->instances == NULL ||
        scene->indices == NULL || scene->nodeCount == 0) return 0;
    stack[0] = 0;
    closest = ray->maxDistance;
    while (stackCount != 0) {
        uint32_t nodeIndex = stack[--stackCount];
        const RayBvhNode *node;
        if (nodeIndex >= scene->nodeCount) return 0;
        node = &scene->nodes[nodeIndex];
        if (!RayIntersectBounds(ray, &node->bounds, closest, NULL)) continue;
        if (node->count != 0) {
            if (node->first > scene->instanceCount ||
                node->count > scene->instanceCount - node->first) return 0;
            for (uint32_t offset = 0; offset < node->count; ++offset) {
                uint32_t instanceIndex = scene->indices[node->first + offset];
                RayHit candidate;
                Ray limited = *ray;
                limited.maxDistance = closest;
                if (instanceIndex >= scene->instanceCount ||
                    !RayInstanceTraceClosest(&scene->instances[instanceIndex],
                                             &limited, &candidate)) continue;
                if (any) return 1;
                candidate.instance = instanceIndex;
                closest = candidate.distance;
                *hit = candidate;
                found = 1;
            }
        } else {
            if (node->left >= scene->nodeCount ||
                node->right >= scene->nodeCount || stackCount + 2 > 64)
                return 0;
            stack[stackCount++] = node->right;
            stack[stackCount++] = node->left;
        }
    }
    return found;
}

int RaySceneTraceClosest(const RayScene *scene, const Ray *ray, RayHit *hit) {
    return TraceScene(scene, ray, hit, 0);
}

int RaySceneTraceAny(const RayScene *scene, const Ray *ray) {
    RayHit ignored;
    return TraceScene(scene, ray, &ignored, 1);
}
