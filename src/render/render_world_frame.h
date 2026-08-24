#ifndef RAGE_RENDER_WORLD_FRAME_H
#define RAGE_RENDER_WORLD_FRAME_H

/* Presentation-time operations over renderer-neutral scene data.  This is
 * intentionally separate from the game adapter: arbitrary-FPS interpolation
 * is a renderer concern, but it must not depend on GTE matrices or packet
 * ordering. */

#include "render_world.h"

float RageRenderLerpAngleDegrees(float from, float to, float t);
void RageRenderInterpolateTransform(const RageRenderTransform *previous,
                                    const RageRenderTransform *current,
                                    float t,
                                    RageRenderTransform *out);
void RageRenderInterpolateCamera(const RageRenderCamera *previous,
                                 const RageRenderCamera *current, float t,
                                 RageRenderCamera *out);

/* Writes one presentable instance for every submitted instance.  `t` is
 * clamped to [0,1]; the source world remains immutable. */
uint32_t RageRenderWorldBuildPresentation(const RageRenderWorld *world,
                                          float t,
                                          RageRenderMeshInstance *out,
                                          uint32_t capacity);

#endif

