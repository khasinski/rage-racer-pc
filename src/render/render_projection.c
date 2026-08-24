#include "render_projection.h"

#include <math.h>

static float RageRadians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

static void RageRotateX(RageRenderVec3 *v, float radians) {
    float y = v->y * cosf(radians) - v->z * sinf(radians);
    float z = v->y * sinf(radians) + v->z * cosf(radians);
    v->y = y; v->z = z;
}

static void RageRotateY(RageRenderVec3 *v, float radians) {
    float x = v->x * cosf(radians) + v->z * sinf(radians);
    float z = -v->x * sinf(radians) + v->z * cosf(radians);
    v->x = x; v->z = z;
}

static void RageRotateZ(RageRenderVec3 *v, float radians) {
    float x = v->x * cosf(radians) - v->y * sinf(radians);
    float y = v->x * sinf(radians) + v->y * cosf(radians);
    v->x = x; v->y = y;
}

static void RageRotateByCameraOrientation(RageRenderVec3 *v,
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

void RageRenderWorldToView(const RageRenderCamera *camera,
                           const RageRenderVec3 *world,
                           RageRenderVec3 *view) {
    *view = *world;
    view->x -= camera->transform.position.x;
    view->y -= camera->transform.position.y;
    view->z -= camera->transform.position.z;
    if (camera->transform.hasOrientation) {
        RageRotateByCameraOrientation(view, &camera->transform.orientation);
    } else {
        /* Inverse of the game's X/Y/Z camera orientation. */
        RageRotateZ(view, -RageRadians(camera->transform.rotation.z));
        RageRotateY(view, -RageRadians(camera->transform.rotation.y));
        RageRotateX(view, -RageRadians(camera->transform.rotation.x));
    }
}

int RageRenderProject(const RageRenderCamera *camera, const RageRenderVec3 *view,
                      float aspect, RageRenderVec3 *clip) {
    float verticalScale;
    if (camera == 0 || view == 0 || clip == 0 || aspect <= 0.0f ||
        view->z < camera->nearPlane || view->z > camera->farPlane) return 0;
    verticalScale = 1.0f / tanf(RageRadians(camera->verticalFovDegrees) * 0.5f);
    clip->x = view->x * verticalScale / (view->z * aspect);
    clip->y = view->y * verticalScale / view->z;
    clip->z = (view->z - camera->nearPlane) /
              (camera->farPlane - camera->nearPlane);
    return 1;
}

