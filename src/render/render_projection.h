#ifndef RAGE_RENDER_PROJECTION_H
#define RAGE_RENDER_PROJECTION_H

#include "render_world.h"

/* Caller-owned snapshot, prepared once per draw build; no global cache shared
 * between the main view, mirror, or interpolated frames. */
typedef struct {
    RageRenderCamera camera;
    float matrix[3][3];
    float cosine[3], sine[3];
    int mode;
} RageRenderViewTransform;

RageRenderViewTransform RenderPrepareView(const RageRenderCamera *camera);
void RenderWorldToViewPrepared(const RageRenderViewTransform *transform,
                              const RageRenderVec3 *world,
                              RageRenderVec3 *view);
float RenderFogFactorPrepared(const RageRenderViewTransform *transform,
                             const RageRenderVec3 *world);

/* Native renderer camera math. The result is conventional view space where
 * positive Z is forward; no PS1 GTE matrix or screen-space quantization is
 * involved. */
void RenderWorldToView(const RageRenderCamera *camera,
                           const RageRenderVec3 *world,
                           RageRenderVec3 *view);
int RenderProject(const RageRenderCamera *camera, const RageRenderVec3 *view,
                      float aspect, RageRenderVec3 *clip);
/* Homogeneous depth terms for a 0..1 depth buffer:
 * clip_z = view_depth * scale + offset, clip_w = view_depth. */
int RenderPerspectiveDepthTerms(const RageRenderCamera *camera,
                                    float *scale, float *offset);
/* Perspective-correct fog weight for a world-space point. */
float RenderFogFactor(const RageRenderCamera *camera,
                          const RageRenderVec3 *world);

#endif
