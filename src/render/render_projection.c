#include "render_projection.h"

#include <math.h>

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
    float length = sqrtf(orientation->x * orientation->x +
                         orientation->y * orientation->y +
                         orientation->z * orientation->z +
                         orientation->w * orientation->w);
    float x = v->x, y = v->y, z = v->z;
    float qx, qy, qz, qw, xx, yy, zz, xy, xz, yz, wx, wy, wz;
    if (length <= 0.0f) return;
    /* The scene stores camera local->world orientation; view uses its inverse. */
    qx = -orientation->x / length; qy = -orientation->y / length;
    qz = -orientation->z / length; qw = orientation->w / length;
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

int RenderProject(const RageRenderCamera *camera, const RageRenderVec3 *view,
                      float aspect, RageRenderVec3 *clip) {
    float verticalScale, depth, depthScale, depthOffset;
    if (camera == 0 || view == 0 || clip == 0 || aspect <= 0.0f ||
        (depth = -view->z) < camera->nearPlane ||
        depth > camera->farPlane) return 0;
    if (!RenderPerspectiveDepthTerms(camera, &depthScale, &depthOffset))
        return 0;
    verticalScale = 1.0f / tanf(Radians(camera->verticalFovDegrees) * 0.5f);
    clip->x = view->x * verticalScale / (depth * aspect);
    clip->y = view->y * verticalScale / depth;
    clip->z = depthScale + depthOffset / depth;
    return 1;
}

int RenderPerspectiveDepthTerms(const RageRenderCamera *camera,
                                    float *scale, float *offset) {
    float range;
    if (camera == 0 || scale == 0 || offset == 0 ||
        camera->nearPlane <= 0.0f ||
        camera->farPlane <= camera->nearPlane) return 0;
    range = camera->farPlane - camera->nearPlane;
    *scale = camera->farPlane / range;
    *offset = -(camera->nearPlane * camera->farPlane) / range;
    return 1;
}

float RenderFogFactor(const RageRenderCamera *camera,
                          const RageRenderVec3 *world) {
    RageRenderVec3 view;
    float depth, inverseNear, inverseFar, factor;
    if (camera == 0 || world == 0 || camera->fogNear <= 0.0f ||
        camera->fogFar <= camera->fogNear) return 0.0f;
    RenderWorldToView(camera, world, &view);
    depth = -view.z;
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
