#include "render_shadow.h"

#include <math.h>
#include <string.h>

const RageRenderVec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION =
    {-0.1f, 1.0f, 0.12f};

static float RenderShadowDot(RageRenderVec3 left, RageRenderVec3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static int RenderShadowVec3IsFinite(RageRenderVec3 value) {
    return isfinite(value.x) && isfinite(value.y) && isfinite(value.z);
}

static int RenderShadowMapIsFinite(const RageRenderShadowMap *shadow) {
    return RenderShadowVec3IsFinite(shadow->position) &&
           RenderShadowVec3IsFinite(shadow->row0) &&
           RenderShadowVec3IsFinite(shadow->row1) &&
           RenderShadowVec3IsFinite(shadow->row2) &&
           isfinite(shadow->scaleX) && isfinite(shadow->scaleY) &&
           isfinite(shadow->depthScale) && isfinite(shadow->depthOffset) &&
           isfinite(shadow->texelWorldSize) &&
           shadow->texelWorldSize > 0.0f;
}

static RageRenderVec3 RenderShadowScale(RageRenderVec3 value,
                                             float scale) {
    RageRenderVec3 out = {value.x * scale, value.y * scale, value.z * scale};
    return out;
}

static RageRenderVec3 RenderShadowAdd(RageRenderVec3 left,
                                           RageRenderVec3 right) {
    RageRenderVec3 out = {
        left.x + right.x, left.y + right.y, left.z + right.z};
    return out;
}

int RenderBuildDirectionalShadowMap(
    const RageRenderVec3 *center, const RageRenderVec3 *lightDirection,
    float extent, uint32_t resolution, RageRenderShadowMap *out) {
    RageRenderVec3 up = {0.0f, 1.0f, 0.0f};
    RageRenderVec3 light;
    RageRenderVec3 right;
    RageRenderVec3 vertical;
    RageRenderVec3 snappedCenter;
    RageRenderShadowMap result;
    double length;
    double rightLength;
    float distance;
    float centerRight;
    float centerVertical;
    float snappedRight;
    float snappedVertical;

    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    if (center == NULL || lightDirection == NULL ||
        !RenderShadowVec3IsFinite(*center) ||
        !RenderShadowVec3IsFinite(*lightDirection) ||
        !isfinite(extent) || extent <= 0.0f || resolution == 0)
        return 0;
    length = sqrt((double)lightDirection->x * lightDirection->x +
                  (double)lightDirection->y * lightDirection->y +
                  (double)lightDirection->z * lightDirection->z);
    if (!isfinite(length) || length <= 0.000001) return 0;
    light = RenderShadowScale(*lightDirection, (float)(1.0 / length));
    right.x = up.y * light.z - up.z * light.y;
    right.y = up.z * light.x - up.x * light.z;
    right.z = up.x * light.y - up.y * light.x;
    rightLength = sqrt((double)RenderShadowDot(right, right));
    if (!isfinite(rightLength) || rightLength <= 0.000001) {
        up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
        right.x = up.y * light.z - up.z * light.y;
        right.y = up.z * light.x - up.x * light.z;
        right.z = up.x * light.y - up.y * light.x;
        rightLength = sqrt((double)RenderShadowDot(right, right));
    }
    if (!isfinite(rightLength) || rightLength <= 0.000001) return 0;
    right = RenderShadowScale(right, (float)(1.0 / rightLength));
    vertical.x = light.y * right.z - light.z * right.y;
    vertical.y = light.z * right.x - light.x * right.z;
    vertical.z = light.x * right.y - light.y * right.x;

    memset(&result, 0, sizeof(result));
    result.row0 = right;
    result.row1 = vertical;
    result.row2 = light;
    result.texelWorldSize = (float)(((double)extent * 2.0) / resolution);
    if (!isfinite(result.texelWorldSize) || result.texelWorldSize <= 0.0f)
        return 0;
    centerRight = RenderShadowDot(*center, right);
    centerVertical = RenderShadowDot(*center, vertical);
    snappedRight = roundf(centerRight / result.texelWorldSize) *
                   result.texelWorldSize;
    snappedVertical = roundf(centerVertical / result.texelWorldSize) *
                      result.texelWorldSize;
    snappedCenter = *center;
    snappedCenter = RenderShadowAdd(
        snappedCenter,
        RenderShadowScale(right, snappedRight - centerRight));
    snappedCenter = RenderShadowAdd(
        snappedCenter,
        RenderShadowScale(vertical, snappedVertical - centerVertical));
    distance = extent * 2.0f;
    result.position = RenderShadowAdd(
        snappedCenter, RenderShadowScale(light, distance));
    result.scaleX = 1.0f / extent;
    result.scaleY = 1.0f / extent;
    result.depthScale = (float)(1.0 / ((double)extent * 4.0));
    result.depthOffset = 0.0f;
    if (!RenderShadowMapIsFinite(&result)) return 0;
    *out = result;
    return 1;
}

void RenderProjectShadowPoint(const RageRenderShadowMap *shadow,
                                  const RageRenderVec3 *point,
                                  RageRenderVec3 *out) {
    RageRenderVec3 relative;
    float depth;
    if (out == NULL) return;
    *out = (RageRenderVec3){0.0f, 0.0f, 0.0f};
    if (shadow == NULL || point == NULL ||
        !RenderShadowMapIsFinite(shadow) ||
        !RenderShadowVec3IsFinite(*point)) return;
    relative.x = point->x - shadow->position.x;
    relative.y = point->y - shadow->position.y;
    relative.z = point->z - shadow->position.z;
    depth = -RenderShadowDot(shadow->row2, relative);
    out->x = RenderShadowDot(shadow->row0, relative) * shadow->scaleX;
    out->y = RenderShadowDot(shadow->row1, relative) * shadow->scaleY;
    out->z = depth * shadow->depthScale + shadow->depthOffset;
    if (!RenderShadowVec3IsFinite(*out))
        *out = (RageRenderVec3){0.0f, 0.0f, 0.0f};
}
