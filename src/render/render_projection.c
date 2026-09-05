#include "render_projection.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

static void RotateX(RageRenderVec3 *v, float radians) {
    float y = v->y * cosf(radians) - v->z * sinf(radians);
    float z = v->y * sinf(radians) + v->z * cosf(radians);
    v->y = y; v->z = z;
}

static void RotateY(RageRenderVec3 *v, float radians) {
    float x = v->x * cosf(radians) + v->z * sinf(radians);
    float z = -v->x * sinf(radians) + v->z * cosf(radians);
    v->x = x; v->z = z;
}

static void RotateZ(RageRenderVec3 *v, float radians) {
    float x = v->x * cosf(radians) - v->y * sinf(radians);
    float y = v->x * sinf(radians) + v->y * cosf(radians);
    v->x = x; v->y = y;
}

static void RotateByCameraOrientation(RageRenderVec3 *v,
                                          const RageRenderQuaternion *orientation) {
    double lengthSquared =
        (double)orientation->x * orientation->x +
        (double)orientation->y * orientation->y +
        (double)orientation->z * orientation->z +
        (double)orientation->w * orientation->w;
    double inverseLength;
    float x = v->x, y = v->y, z = v->z;
    float qx, qy, qz, qw, xx, yy, zz, xy, xz, yz, wx, wy, wz;
    if (!isfinite(lengthSquared) || lengthSquared <= 0.0) return;
    inverseLength = 1.0 / sqrt(lengthSquared);
    qx = (float)(-(double)orientation->x * inverseLength);
    qy = (float)(-(double)orientation->y * inverseLength);
    qz = (float)(-(double)orientation->z * inverseLength);
    qw = (float)((double)orientation->w * inverseLength);
    /* The scene stores camera local->world orientation; view uses its inverse. */
    xx = qx * qx; yy = qy * qy; zz = qz * qz;
    xy = qx * qy; xz = qx * qz; yz = qy * qz;
    wx = qw * qx; wy = qw * qy; wz = qw * qz;
    v->x = (1.0f - 2.0f * (yy + zz)) * x + 2.0f * (xy - wz) * y +
           2.0f * (xz + wy) * z;
    v->y = 2.0f * (xy + wz) * x + (1.0f - 2.0f * (xx + zz)) * y +
           2.0f * (yz - wx) * z;
    v->z = 2.0f * (xz - wy) * x + 2.0f * (yz + wx) * y +
           (1.0f - 2.0f * (xx + yy)) * z;
}

void RenderWorldToView(const RageRenderCamera *camera,
                           const RageRenderVec3 *world,
                           RageRenderVec3 *view) {
    if (view == NULL) return;
    *view = (RageRenderVec3){0.0f, 0.0f, 0.0f};
    if (camera == NULL || world == NULL) return;
    *view = *world;
    view->x -= camera->transform.position.x;
    view->y -= camera->transform.position.y;
    view->z -= camera->transform.position.z;
    if (camera->transform.hasOrientation) {
        RotateByCameraOrientation(view, &camera->transform.orientation);
    } else {
        /* Inverse of the game's X/Y/Z camera orientation. */
        RotateZ(view, -Radians(camera->transform.rotation.z));
        RotateY(view, -Radians(camera->transform.rotation.y));
        RotateX(view, -Radians(camera->transform.rotation.x));
    }
}

RageRenderViewTransform RenderPrepareView(const RageRenderCamera *camera) {
    RageRenderViewTransform result = {0};
    result.mode = -1;
    if (camera == NULL) return result;
    result.camera = *camera;
    result.mode = 0;
    if (camera->transform.hasOrientation) {
        const RageRenderQuaternion *q = &camera->transform.orientation;
        double lengthSquared = (double)q->x * q->x + (double)q->y * q->y +
                               (double)q->z * q->z + (double)q->w * q->w;
        if (!isfinite(lengthSquared) || lengthSquared <= 0.0) return result;
        double inverseLength = 1.0 / sqrt(lengthSquared);
        float qx = (float)(-(double)q->x * inverseLength);
        float qy = (float)(-(double)q->y * inverseLength);
        float qz = (float)(-(double)q->z * inverseLength);
        float qw = (float)((double)q->w * inverseLength);
        float xx = qx * qx, yy = qy * qy, zz = qz * qz;
        float xy = qx * qy, xz = qx * qz, yz = qy * qz;
        float wx = qw * qx, wy = qw * qy, wz = qw * qz;
        result.matrix[0][0] = 1.0f - 2.0f * (yy + zz);
        result.matrix[0][1] = 2.0f * (xy - wz);
        result.matrix[0][2] = 2.0f * (xz + wy);
        result.matrix[1][0] = 2.0f * (xy + wz);
        result.matrix[1][1] = 1.0f - 2.0f * (xx + zz);
        result.matrix[1][2] = 2.0f * (yz - wx);
        result.matrix[2][0] = 2.0f * (xz - wy);
        result.matrix[2][1] = 2.0f * (yz + wx);
        result.matrix[2][2] = 1.0f - 2.0f * (xx + yy);
        result.mode = 1;
    } else {
        float angles[3] = {-Radians(camera->transform.rotation.x),
                           -Radians(camera->transform.rotation.y),
                           -Radians(camera->transform.rotation.z)};
        for (int i = 0; i < 3; ++i) {
            result.cosine[i] = cosf(angles[i]);
            result.sine[i] = sinf(angles[i]);
        }
        result.mode = 2;
    }
    return result;
}

void RenderWorldToViewPrepared(const RageRenderViewTransform *p,
                              const RageRenderVec3 *world,
                              RageRenderVec3 *view) {
    if (view == NULL) return;
    *view = (RageRenderVec3){0};
    if (p == NULL || p->mode < 0 || world == NULL) return;
    *view = *world;
    view->x -= p->camera.transform.position.x;
    view->y -= p->camera.transform.position.y;
    view->z -= p->camera.transform.position.z;
    if (p->mode == 1) {
        float x = view->x, y = view->y, z = view->z;
        view->x = p->matrix[0][0] * x + p->matrix[0][1] * y + p->matrix[0][2] * z;
        view->y = p->matrix[1][0] * x + p->matrix[1][1] * y + p->matrix[1][2] * z;
        view->z = p->matrix[2][0] * x + p->matrix[2][1] * y + p->matrix[2][2] * z;
    } else if (p->mode == 2) {
        /* Keep separate Z/Y/X rotations: combining matrices changes rounding. */
        float x = view->x * p->cosine[2] - view->y * p->sine[2];
        float y = view->x * p->sine[2] + view->y * p->cosine[2];
        view->x = x; view->y = y;
        x = view->x * p->cosine[1] + view->z * p->sine[1];
        float z = -view->x * p->sine[1] + view->z * p->cosine[1];
        view->x = x; view->z = z;
        y = view->y * p->cosine[0] - view->z * p->sine[0];
        z = view->y * p->sine[0] + view->z * p->cosine[0];
        view->y = y; view->z = z;
    }
}

int RenderProject(const RageRenderCamera *camera, const RageRenderVec3 *view,
                  float aspect, RageRenderVec3 *clip) {
    float depthScale, depthOffset;
    double verticalScale, depth;
    double x, y, z;

    if (clip == NULL) return 0;
    *clip = (RageRenderVec3){0.0f, 0.0f, 0.0f};
    if (camera == NULL || view == NULL ||
        !isfinite(aspect) || aspect <= 0.0f ||
        !isfinite(camera->verticalFovDegrees) ||
        camera->verticalFovDegrees <= 0.0f ||
        camera->verticalFovDegrees >= 180.0f ||
        !isfinite(view->x) || !isfinite(view->y) || !isfinite(view->z) ||
        (depth = -view->z) < camera->nearPlane ||
        depth > camera->farPlane) return 0;
    if (!RenderPerspectiveDepthTerms(camera, &depthScale, &depthOffset))
        return 0;
    verticalScale =
        1.0 / tan((double)camera->verticalFovDegrees *
                  (3.14159265358979323846 / 180.0) * 0.5);
    x = (double)view->x * verticalScale / (depth * aspect);
    y = (double)view->y * verticalScale / depth;
    z = (double)depthScale + (double)depthOffset / depth;
    if (!isfinite(x) || !isfinite(y) || !isfinite(z) ||
        fabs(x) > FLT_MAX || fabs(y) > FLT_MAX || fabs(z) > FLT_MAX) {
        return 0;
    }
    clip->x = (float)x;
    clip->y = (float)y;
    clip->z = (float)z;
    return 1;
}

int RenderPerspectiveDepthTerms(const RageRenderCamera *camera,
                                float *scale, float *offset) {
    double range;
    double resultScale;
    double resultOffset;

    if (scale == NULL || offset == NULL) return 0;
    *scale = 0.0f;
    *offset = 0.0f;
    if (camera == NULL ||
        !isfinite(camera->nearPlane) || !isfinite(camera->farPlane) ||
        camera->nearPlane <= 0.0f ||
        camera->farPlane <= camera->nearPlane) return 0;
    range = (double)camera->farPlane - camera->nearPlane;
    resultScale = (double)camera->farPlane / range;
    resultOffset = -((double)camera->nearPlane * camera->farPlane) / range;
    if (!isfinite(resultScale) || !isfinite(resultOffset) ||
        fabs(resultScale) > FLT_MAX || fabs(resultOffset) > FLT_MAX) {
        return 0;
    }
    *scale = (float)resultScale;
    *offset = (float)resultOffset;
    return 1;
}

float RenderFogFactor(const RageRenderCamera *camera,
                      const RageRenderVec3 *world) {
    RageRenderVec3 view;
    float depth, inverseNear, inverseFar, factor;
    if (camera == NULL || world == NULL ||
        !isfinite(camera->fogNear) || !isfinite(camera->fogFar) ||
        !isfinite(world->x) || !isfinite(world->y) || !isfinite(world->z) ||
        camera->fogNear <= 0.0f ||
        camera->fogFar <= camera->fogNear) return 0.0f;
    RenderWorldToView(camera, world, &view);
    depth = -view.z;
    if (!isfinite(depth)) return 0.0f;
    if (depth <= camera->fogNear) return 0.0f;
    if (depth >= camera->fogFar) return 1.0f;
    /* Perspective fog interpolates in reciprocal depth. This retains the
     * authored look while remaining ordinary renderer-neutral scene math. */
    inverseNear = 1.0f / camera->fogNear;
    inverseFar = 1.0f / camera->fogFar;
    factor = (inverseNear - 1.0f / depth) / (inverseNear - inverseFar);
    if (factor < 0.0f) return 0.0f;
    if (factor > 1.0f) return 1.0f;
    return factor;
}

float RenderFogFactorPrepared(const RageRenderViewTransform *transform,
                             const RageRenderVec3 *world) {
    const RageRenderCamera *camera = transform != NULL && transform->mode >= 0
        ? &transform->camera : NULL;
    RageRenderVec3 view;
    float depth, inverseNear, inverseFar, factor;
    if (camera == NULL || world == NULL ||
        !isfinite(camera->fogNear) || !isfinite(camera->fogFar) ||
        !isfinite(world->x) || !isfinite(world->y) || !isfinite(world->z) ||
        camera->fogNear <= 0.0f ||
        camera->fogFar <= camera->fogNear) return 0.0f;
    RenderWorldToViewPrepared(transform, world, &view);
    depth = -view.z;
    if (!isfinite(depth)) return 0.0f;
    if (depth <= camera->fogNear) return 0.0f;
    if (depth >= camera->fogFar) return 1.0f;
    /* Perspective fog interpolates in reciprocal depth. This retains the
     * authored look while remaining ordinary renderer-neutral scene math. */
    inverseNear = 1.0f / camera->fogNear;
    inverseFar = 1.0f / camera->fogFar;
    factor = (inverseNear - 1.0f / depth) / (inverseNear - inverseFar);
    if (factor < 0.0f) return 0.0f;
    if (factor > 1.0f) return 1.0f;
    return factor;
}
