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

/* The sun is shadowed by nested cascades centred on the player. Each is a
 * texel-snapped orthographic map, so a cascade neither shimmers nor depends
 * on the camera direction, and the rear-view mirror sees the same coverage.
 * The tiles share one atlas, laid out two by two. */
enum {
    RAGE_RENDER_SHADOW_CASCADES = 4,
    RAGE_RENDER_SHADOW_TILE_STANDARD = 1024,
    RAGE_RENDER_SHADOW_TILE_HIGH = 2048,
};

typedef struct RenderShadowCascades {
    RenderShadowMap map[RAGE_RENDER_SHADOW_CASCADES];
    /* World position to atlas u, v and depth, one affine row each: the
     * fragment shader evaluates dot(row.xyz, position) + row.w. */
    float rows[RAGE_RENDER_SHADOW_CASCADES][3][4];
} RenderShadowCascades;

/* A high, slightly offset sun keeps vehicle contact shadows close to their
 * casters while retaining a readable direction. */
extern const Vec3 RAGE_RENDER_DEFAULT_LIGHT_DIRECTION;

/* Builds a texel-snapped orthographic camera looking from the light toward
 * `center`. `lightDirection` points from a surface toward the light. */
int RenderBuildDirectionalShadowMap(
    const Vec3 *center, const Vec3 *lightDirection,
    float extent, uint32_t resolution, RenderShadowMap *out);

/* Fits every cascade around `center`. Returns 0 and clears `out` when the
 * centre or light direction is unusable. */
int RenderBuildShadowCascades(const Vec3 *center, const Vec3 *lightDirection,
                              uint32_t tileResolution,
                              RenderShadowCascades *out);

/* Radius around the cascade centre inside which an object can shadow the
 * outer cascade; objects farther out are clipped by every cascade anyway. */
float RenderShadowCasterRadius(void);

void RenderProjectShadowPoint(const RenderShadowMap *shadow,
                                  const Vec3 *point,
                                  Vec3 *out);

#endif
