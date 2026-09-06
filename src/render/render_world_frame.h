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
 * RAGE_RENDER_PRESENTATION_MAX_INSTANCES are rejected. Insufficient output
 * capacity or an already-overflowed source rejects the entire result and
 * leaves output unchanged, rather than publishing a truncated scene.
 * Zero denotes empty or rejected output. */
uint32_t RenderWorldBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity);
/* Explicit success result, including a valid empty scene. Failure preserves
 * both output storage and *count; success publishes the complete count. */
int RenderWorldTryBuildSynchronizedPresentation(
    const RageRenderWorld *previous, const RageRenderWorld *current, float t,
    RageRenderMeshInstance *out, uint32_t capacity, uint32_t *count);

#endif
