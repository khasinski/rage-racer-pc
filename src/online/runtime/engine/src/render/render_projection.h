#ifndef RAGE_RENDER_PROJECTION_H
#define RAGE_RENDER_PROJECTION_H

#include "render_world.h"

/* Caller-owned snapshot, prepared once per draw build; no global cache shared
 * between the main view, mirror, or interpolated frames. */
typedef struct {
    RenderCamera camera;
    float matrix[3][3];
    float cosine[3], sine[3];
    int mode;
} RenderViewTransform;

RenderViewTransform RenderPrepareView(const RenderCamera *camera);
void RenderWorldToViewPrepared(const RenderViewTransform *transform,
                              const Vec3 *world,
                              Vec3 *view);
float RenderFogFactorPrepared(const RenderViewTransform *transform,
                             const Vec3 *world);

/* Homogeneous depth terms for a 0..1 depth buffer:
 * clip_z = view_depth * scale + offset, clip_w = view_depth. */
/* GPU projection scales, preserving float operation order for valid cameras.
 * Rejects invalid or unrepresentable projection before division/upload. */
int RenderPerspectiveScales(const RenderCamera *camera, float aspect,
                            float *horizontal, float *vertical);

int RenderPerspectiveDepthTerms(const RenderCamera *camera,
                                    float *scale, float *offset);

#endif
