#include "render_projection.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

static float Radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

RenderViewTransform RenderPrepareView(const RenderCamera *camera) {
    RenderViewTransform result = {0};
    result.mode = -1;
    if (camera == NULL) return result;
    result.camera = *camera;
    result.mode = 0;
    if (camera->transform.hasOrientation) {
        const Quaternion *q = &camera->transform.orientation;
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

void RenderWorldToViewPrepared(const RenderViewTransform *p,
                              const Vec3 *world,
                              Vec3 *view) {
    if (view == NULL) return;
    *view = (Vec3){0};
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

int RenderPerspectiveScales(const RenderCamera *camera, float aspect,
                            float *horizontal, float *vertical) {
    float tangent, scale;
    if (!horizontal || !vertical) return 0;
    *horizontal = *vertical = 0.0f;
    if (!camera || !isfinite(aspect) || aspect <= 0.0f ||
        !isfinite(camera->verticalFovDegrees) ||
        camera->verticalFovDegrees <= 0.0f ||
        camera->verticalFovDegrees >= 180.0f) return 0;
    tangent = tanf(camera->verticalFovDegrees * 0.008726646259971648f);
    if (!isfinite(tangent) || tangent <= 0.0f ||
        1.0 / (double)tangent > FLT_MAX) return 0;
    scale = 1.0f / tangent;
    if ((double)scale / aspect > FLT_MAX ||
        (double)scale / aspect < FLT_TRUE_MIN) return 0;
    *horizontal = scale / aspect;
    *vertical = scale;
    return 1;
}

int RenderPerspectiveDepthTerms(const RenderCamera *camera,
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

float RenderFogFactorPrepared(const RenderViewTransform *transform,
                             const Vec3 *world) {
    const RenderCamera *camera = transform != NULL && transform->mode >= 0
        ? &transform->camera : NULL;
    Vec3 view;
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
