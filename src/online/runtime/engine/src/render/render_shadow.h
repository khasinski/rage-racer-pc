#ifndef RAGE_RENDER_SHADOW_H
#define RAGE_RENDER_SHADOW_H

#include <stdint.h>

#include "render_world.h"

typedef struct RenderShadowMap {
    Vec3 position;
    Vec3 row0;
    Vec3 row1;
    Vec3 row2;
    float scaleX;
    float scaleY;
    float depthScale;
    float depthOffset;
    float texelWorldSize;
} RenderShadowMap;

/* One stable map follows the player and covers two terrain cells in every
 * direction. At 4096 samples this keeps vehicle silhouettes at two world
 * units per texel without introducing cascades or camera-relative shimmer. */
enum { RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION = 4096 };
static const float RAGE_RENDER_VEHICLE_SHADOW_EXTENT = 4096.0f;

/* A high, slightly offset sun keeps vehicle contact shadows close to their
 * casters while retaining a readable direction. */
extern const Vec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;

/* Follow the explicit scene subject, or the camera when none is published. */
Vec3 RenderShadowCenter(const RenderWorld *world);

/* Builds a texel-snapped orthographic camera looking from the light toward
 * `center`. `lightDirection` points from a surface toward the light. */
int RenderBuildDirectionalShadowMap(
    const Vec3 *center, const Vec3 *lightDirection,
    float extent, uint32_t resolution, RenderShadowMap *out);

#endif
