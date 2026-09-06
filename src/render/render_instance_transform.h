#ifndef RAGE_RENDER_INSTANCE_TRANSFORM_H
#define RAGE_RENDER_INSTANCE_TRANSFORM_H

#include "render_world.h"
#include <math.h>

/* Prepared instance-local transform, independent of mesh storage and cameras.
 * Preserve the legacy Euler operation order and normalized quaternion path.
 * Normal vectors use rotation only: no inverse-scale correction is implied. */
static inline float RenderInstanceRadians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

typedef struct RageRenderInstanceTransform {
    RageRenderVec3 position;
    RageRenderVec3 scale;
    float cx, sx, cy, sy, cz, sz;
    float matrix[3][3];
    int useMatrix;
} RageRenderInstanceTransform;

static inline RageRenderInstanceTransform RenderPrepareInstanceTransform(
    const RageRenderTransform *transform) {
    RageRenderInstanceTransform basis = {0};
    basis.position = transform->position;
    basis.scale = transform->scale;
    if (transform->hasOrientation) {
        const RageRenderQuaternion *q = &transform->orientation;
        double lengthSquared =
            (double)q->x * q->x + (double)q->y * q->y +
            (double)q->z * q->z + (double)q->w * q->w;
        if (isfinite(lengthSquared) && lengthSquared > 0.0) {
            double inverseLength = 1.0 / sqrt(lengthSquared);
            float xq = (float)((double)q->x * inverseLength);
            float yq = (float)((double)q->y * inverseLength);
            float zq = (float)((double)q->z * inverseLength);
            float wq = (float)((double)q->w * inverseLength);
            basis.matrix[0][0] = 1.0f - 2.0f * (yq * yq + zq * zq);
            basis.matrix[0][1] = 2.0f * (xq * yq - zq * wq);
            basis.matrix[0][2] = 2.0f * (xq * zq + yq * wq);
            basis.matrix[1][0] = 2.0f * (xq * yq + zq * wq);
            basis.matrix[1][1] = 1.0f - 2.0f * (xq * xq + zq * zq);
            basis.matrix[1][2] = 2.0f * (yq * zq - xq * wq);
            basis.matrix[2][0] = 2.0f * (xq * zq - yq * wq);
            basis.matrix[2][1] = 2.0f * (yq * zq + xq * wq);
            basis.matrix[2][2] = 1.0f - 2.0f * (xq * xq + yq * yq);
            basis.useMatrix = 1;
        }
    }
    if (!basis.useMatrix) {
        float x = RenderInstanceRadians(transform->rotation.x);
        float y = RenderInstanceRadians(transform->rotation.y);
        float z = RenderInstanceRadians(transform->rotation.z);
        basis.cx = cosf(x); basis.sx = sinf(x);
        basis.cy = cosf(y); basis.sy = sinf(y);
        basis.cz = cosf(z); basis.sz = sinf(z);
    }
    return basis;
}

static inline RageRenderVec3 RenderRotateInstanceVector(
    const RageRenderInstanceTransform *basis, RageRenderVec3 vector) {
    float x;
    if (basis->useMatrix) {
        RageRenderVec3 rotated;
        rotated.x = basis->matrix[0][0] * vector.x +
                    basis->matrix[0][1] * vector.y +
                    basis->matrix[0][2] * vector.z;
        rotated.y = basis->matrix[1][0] * vector.x +
                    basis->matrix[1][1] * vector.y +
                    basis->matrix[1][2] * vector.z;
        rotated.z = basis->matrix[2][0] * vector.x +
                    basis->matrix[2][1] * vector.y +
                    basis->matrix[2][2] * vector.z;
        return rotated;
    }
    float y = vector.y * basis->cx - vector.z * basis->sx;
    float z = vector.y * basis->sx + vector.z * basis->cx;
    vector.y = y;
    vector.z = z;
    x = vector.x * basis->cy + vector.z * basis->sy;
    z = -vector.x * basis->sy + vector.z * basis->cy;
    vector.x = x;
    vector.z = z;
    x = vector.x * basis->cz - vector.y * basis->sz;
    y = vector.x * basis->sz + vector.y * basis->cz;
    vector.x = x;
    vector.y = y;
    return vector;
}

static inline RageRenderVec3 RenderTransformInstancePoint(
    const RageRenderInstanceTransform *basis, RageRenderVec3 point) {
    point.x *= basis->scale.x;
    point.y *= basis->scale.y;
    point.z *= basis->scale.z;
    point = RenderRotateInstanceVector(basis, point);
    point.x += basis->position.x;
    point.y += basis->position.y;
    point.z += basis->position.z;
    return point;
}

#endif
