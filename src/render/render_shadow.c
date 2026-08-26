#include "render_shadow.h"

#include <math.h>
#include <string.h>

const RageRenderVec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION =
    {-0.1f, 1.0f, 0.12f};

static float RageRenderShadowDot(RageRenderVec3 left, RageRenderVec3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static RageRenderVec3 RageRenderShadowScale(RageRenderVec3 value,
                                             float scale) {
    RageRenderVec3 out = {value.x * scale, value.y * scale, value.z * scale};
    return out;
}

static RageRenderVec3 RageRenderShadowAdd(RageRenderVec3 left,
                                           RageRenderVec3 right) {
    RageRenderVec3 out = {
        left.x + right.x, left.y + right.y, left.z + right.z};
    return out;
}

int RageRenderBuildDirectionalShadowMap(
    const RageRenderVec3 *center, const RageRenderVec3 *lightDirection,
    float extent, uint32_t resolution, RageRenderShadowMap *out) {
    RageRenderVec3 up = {0.0f, 1.0f, 0.0f};
    RageRenderVec3 light;
    RageRenderVec3 right;
    RageRenderVec3 vertical;
    RageRenderVec3 snappedCenter;
    float length;
    float rightLength;
    float distance;
    float centerRight;
    float centerVertical;
    float snappedRight;
    float snappedVertical;

    if (center == NULL || lightDirection == NULL || out == NULL ||
        extent <= 0.0f || resolution == 0)
        return 0;
    length = sqrtf(RageRenderShadowDot(*lightDirection, *lightDirection));
    if (length <= 0.000001f) return 0;
    light = RageRenderShadowScale(*lightDirection, 1.0f / length);
    right.x = up.y * light.z - up.z * light.y;
    right.y = up.z * light.x - up.x * light.z;
    right.z = up.x * light.y - up.y * light.x;
    rightLength = sqrtf(RageRenderShadowDot(right, right));
    if (rightLength <= 0.000001f) {
        up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
        right.x = up.y * light.z - up.z * light.y;
        right.y = up.z * light.x - up.x * light.z;
        right.z = up.x * light.y - up.y * light.x;
        rightLength = sqrtf(RageRenderShadowDot(right, right));
    }
    right = RageRenderShadowScale(right, 1.0f / rightLength);
    vertical.x = light.y * right.z - light.z * right.y;
    vertical.y = light.z * right.x - light.x * right.z;
    vertical.z = light.x * right.y - light.y * right.x;

    memset(out, 0, sizeof(*out));
    out->row0 = right;
    out->row1 = vertical;
    out->row2 = light;
    out->texelWorldSize = (extent * 2.0f) / (float)resolution;
    centerRight = RageRenderShadowDot(*center, right);
    centerVertical = RageRenderShadowDot(*center, vertical);
    snappedRight = roundf(centerRight / out->texelWorldSize) *
                   out->texelWorldSize;
    snappedVertical = roundf(centerVertical / out->texelWorldSize) *
                      out->texelWorldSize;
    snappedCenter = *center;
    snappedCenter = RageRenderShadowAdd(
        snappedCenter,
        RageRenderShadowScale(right, snappedRight - centerRight));
    snappedCenter = RageRenderShadowAdd(
        snappedCenter,
        RageRenderShadowScale(vertical, snappedVertical - centerVertical));
    distance = extent * 2.0f;
    out->position = RageRenderShadowAdd(
        snappedCenter, RageRenderShadowScale(light, distance));
    out->scaleX = 1.0f / extent;
    out->scaleY = 1.0f / extent;
    out->depthScale = 1.0f / (extent * 4.0f);
    out->depthOffset = 0.0f;
    return 1;
}

void RageRenderProjectShadowPoint(const RageRenderShadowMap *shadow,
                                  const RageRenderVec3 *point,
                                  RageRenderVec3 *out) {
    RageRenderVec3 relative;
    float depth;
    if (shadow == NULL || point == NULL || out == NULL) return;
    relative.x = point->x - shadow->position.x;
    relative.y = point->y - shadow->position.y;
    relative.z = point->z - shadow->position.z;
    depth = -RageRenderShadowDot(shadow->row2, relative);
    out->x = RageRenderShadowDot(shadow->row0, relative) * shadow->scaleX;
    out->y = RageRenderShadowDot(shadow->row1, relative) * shadow->scaleY;
    out->z = depth * shadow->depthScale + shadow->depthOffset;
}
