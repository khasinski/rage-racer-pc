#include "ray_scene.h"

#include <math.h>
#include <stddef.h>

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
