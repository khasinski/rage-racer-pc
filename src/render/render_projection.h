#ifndef RAGE_RENDER_PROJECTION_H
#define RAGE_RENDER_PROJECTION_H

#include "render_world.h"

/* Native renderer camera math. The result is conventional view space where
 * positive Z is forward; no PS1 GTE matrix or screen-space quantization is
 * involved. */
void RageRenderWorldToView(const RageRenderCamera *camera,
                           const RageRenderVec3 *world,
                           RageRenderVec3 *view);
int RageRenderProject(const RageRenderCamera *camera, const RageRenderVec3 *view,
                      float aspect, RageRenderVec3 *clip);
/* Homogeneous depth terms for a 0..1 depth buffer:
 * clip_z = view_depth * scale + offset, clip_w = view_depth. */
int RageRenderPerspectiveDepthTerms(const RageRenderCamera *camera,
                                    float *scale, float *offset);
/* Perspective-correct fog weight for a world-space point. */
float RageRenderFogFactor(const RageRenderCamera *camera,
                          const RageRenderVec3 *world);

#endif
