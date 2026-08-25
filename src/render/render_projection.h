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
/* Perspective-correct fog weight for a world-space point. */
float RageRenderFogFactor(const RageRenderCamera *camera,
                          const RageRenderVec3 *world);

#endif
