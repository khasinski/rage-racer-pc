#ifndef RAGE_RENDER_WORLD_FRAME_H
#define RAGE_RENDER_WORLD_FRAME_H

/* Presentation-time operations over renderer-neutral scene data.  This is
 * intentionally separate from the game adapter: arbitrary-FPS interpolation
 * is a renderer concern, but it must not depend on GTE matrices or packet
 * ordering. */

#include "render_world.h"

enum { RAGE_RENDER_PRESENTATION_MAX_INSTANCES = 4096 };

float RenderLerpAngleDegrees(float from, float to, float t);
void RenderInterpolateTransform(const RageRenderTransform *previous,
                                    const RageRenderTransform *current,
                                    float t,
                                    RageRenderTransform *out);
void RenderInterpolateCamera(const RageRenderCamera *previous,
                                 const RageRenderCamera *current, float t,
                                 RageRenderCamera *out);

/* Build the frame shown alongside a previous-frame compatibility snapshot.
 * Static world instances retain the normal producer-supplied interpolation,
 * while vehicles keep the previous frame's exact model/visibility set and
 * move toward matching current transforms. Worlds larger than
 * RAGE_RENDER_PRESENTATION_MAX_INSTANCES are rejected. */
uint32_t RenderWorldBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity);

#endif
