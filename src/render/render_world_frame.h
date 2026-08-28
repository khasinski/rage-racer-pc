#ifndef RAGE_RENDER_WORLD_FRAME_H
#define RAGE_RENDER_WORLD_FRAME_H

/* Presentation-time operations over renderer-neutral scene data.  This is
 * intentionally separate from the game adapter: arbitrary-FPS interpolation
 * is a renderer concern, but it must not depend on GTE matrices or packet
 * ordering. */

#include "render_world.h"

float RenderLerpAngleDegrees(float from, float to, float t);
void RenderInterpolateTransform(const RageRenderTransform *previous,
                                    const RageRenderTransform *current,
                                    float t,
                                    RageRenderTransform *out);
void RenderInterpolateCamera(const RageRenderCamera *previous,
                                 const RageRenderCamera *current, float t,
                                 RageRenderCamera *out);

/* Writes one presentable instance for every submitted instance.  `t` is
 * clamped to [0,1]; the source world remains immutable. */
uint32_t RenderWorldBuildPresentation(const RageRenderWorld *world,
                                          float t,
                                          RageRenderMeshInstance *out,
                                          uint32_t capacity);

/* Build the frame shown alongside a previous-frame compatibility snapshot.
 * Static world instances retain the normal producer-supplied interpolation,
 * while vehicles keep the previous frame's exact model/visibility set and
 * move toward matching current transforms. */
uint32_t RenderWorldBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity);

#endif
