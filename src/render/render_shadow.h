#ifndef RAGE_RENDER_SHADOW_H
#define RAGE_RENDER_SHADOW_H

#include <stdint.h>

#include "render_world.h"

typedef struct RageRenderShadowMap {
    RageRenderVec3 position;
    RageRenderVec3 row0;
    RageRenderVec3 row1;
    RageRenderVec3 row2;
    float scaleX;
    float scaleY;
    float depthScale;
    float depthOffset;
    float texelWorldSize;
} RageRenderShadowMap;

/* A high, slightly offset sun keeps vehicle contact shadows close to their
 * casters while retaining a readable direction. */
extern const RageRenderVec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;

/* Builds a texel-snapped orthographic camera looking from the light toward
 * `center`. `lightDirection` points from a surface toward the light. */
int RageRenderBuildDirectionalShadowMap(
    const RageRenderVec3 *center, const RageRenderVec3 *lightDirection,
    float extent, uint32_t resolution, RageRenderShadowMap *out);

void RageRenderProjectShadowPoint(const RageRenderShadowMap *shadow,
                                  const RageRenderVec3 *point,
                                  RageRenderVec3 *out);

#endif
