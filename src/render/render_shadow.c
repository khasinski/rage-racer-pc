#include "render_shadow.h"

#include <math.h>
#include <string.h>

const Vec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION =
    {-0.34f, 1.0f, 0.42f};

static float RenderShadowDot(Vec3 left, Vec3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static int RenderShadowVec3IsFinite(Vec3 value) {
    return isfinite(value.x) && isfinite(value.y) && isfinite(value.z);
}

static int RenderShadowMapIsFinite(const RenderShadowMap *shadow) {
    return RenderShadowVec3IsFinite(shadow->position) &&
           RenderShadowVec3IsFinite(shadow->row0) &&
           RenderShadowVec3IsFinite(shadow->row1) &&
           RenderShadowVec3IsFinite(shadow->row2) &&
           isfinite(shadow->scaleX) && isfinite(shadow->scaleY) &&
           isfinite(shadow->depthScale) && isfinite(shadow->depthOffset) &&
           isfinite(shadow->texelWorldSize) &&
           shadow->texelWorldSize > 0.0f;
}

static Vec3 RenderShadowScale(Vec3 value,
                                             float scale) {
    Vec3 out = {value.x * scale, value.y * scale, value.z * scale};
    return out;
}

static Vec3 RenderShadowAdd(Vec3 left,
                                           Vec3 right) {
    Vec3 out = {
        left.x + right.x, left.y + right.y, left.z + right.z};
    return out;
}

int RenderBuildDirectionalShadowMap(
    const Vec3 *center, const Vec3 *lightDirection,
    float extent, uint32_t resolution, RenderShadowMap *out) {
    Vec3 up = {0.0f, 1.0f, 0.0f};
    Vec3 light;
    Vec3 right;
    Vec3 vertical;
    Vec3 snappedCenter;
    RenderShadowMap result;
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

void RenderProjectShadowPoint(const RenderShadowMap *shadow,
                                  const Vec3 *point,
                                  Vec3 *out) {
    Vec3 relative;
    float depth;
    if (out == NULL) return;
    *out = (Vec3){0.0f, 0.0f, 0.0f};
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
        *out = (Vec3){0.0f, 0.0f, 0.0f};
}

/* Half-widths of the nested cascades in world units. With 2048 texels per
 * tile these resolve a half, two, six and sixteen units per texel; 1024
 * halves each resolution. */
static const float SHADOW_CASCADE_EXTENT[RAGE_RENDER_SHADOW_CASCADES] = {
    512.0f, 2048.0f, 6144.0f, 16384.0f};

static void PackCascadeRows(const RenderShadowMap *shadow, unsigned cascade,
                            float rows[3][4]) {
    const float tileU = (float)(cascade & 1u);
    const float tileV = (float)(cascade >> 1);
    const float quarterX = 0.25f * shadow->scaleX;
    const float quarterY = 0.25f * shadow->scaleY;
    const Vec3 *position = &shadow->position;
    rows[0][0] = shadow->row0.x * quarterX;
    rows[0][1] = shadow->row0.y * quarterX;
    rows[0][2] = shadow->row0.z * quarterX;
    rows[0][3] = (tileU + 0.5f) * 0.5f -
                 RenderShadowDot(shadow->row0, *position) * quarterX;
    rows[1][0] = -shadow->row1.x * quarterY;
    rows[1][1] = -shadow->row1.y * quarterY;
    rows[1][2] = -shadow->row1.z * quarterY;
    rows[1][3] = (tileV + 0.5f) * 0.5f +
                 RenderShadowDot(shadow->row1, *position) * quarterY;
    rows[2][0] = -shadow->row2.x * shadow->depthScale;
    rows[2][1] = -shadow->row2.y * shadow->depthScale;
    rows[2][2] = -shadow->row2.z * shadow->depthScale;
    rows[2][3] = RenderShadowDot(shadow->row2, *position) *
                     shadow->depthScale + shadow->depthOffset;
}

float RenderShadowCasterRadius(void) {
    return SHADOW_CASCADE_EXTENT[RAGE_RENDER_SHADOW_CASCADES - 1];
}

int RenderBuildShadowCascades(const Vec3 *center, const Vec3 *lightDirection,
                              uint32_t tileResolution,
                              RenderShadowCascades *out) {
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    for (unsigned cascade = 0; cascade < RAGE_RENDER_SHADOW_CASCADES;
         ++cascade) {
        if (!RenderBuildDirectionalShadowMap(
                center, lightDirection, SHADOW_CASCADE_EXTENT[cascade],
                tileResolution, &out->map[cascade])) {
            memset(out, 0, sizeof(*out));
            return 0;
        }
        PackCascadeRows(&out->map[cascade], cascade, out->rows[cascade]);
    }
    return 1;
}
