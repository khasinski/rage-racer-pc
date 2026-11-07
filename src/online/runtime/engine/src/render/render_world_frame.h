#ifndef RAGE_RENDER_WORLD_FRAME_H
#define RAGE_RENDER_WORLD_FRAME_H

/* Presentation-time operations over renderer-neutral scene data.  This is
 * intentionally separate from the game adapter: arbitrary-FPS interpolation
 * is a renderer concern, but it must not depend on GTE matrices or packet
 * ordering. */

#include "render_world.h"

enum { RAGE_RENDER_PRESENTATION_MAX_INSTANCES = 4096 };

float RenderLerpAngleDegrees(float from, float to, float t);
void RenderInterpolateTransform(const RenderTransform *previous,
                                    const RenderTransform *current,
                                    float t,
                                    RenderTransform *out);
void RenderInterpolateCamera(const RenderCamera *previous,
                                 const RenderCamera *current, float t,
                                 RenderCamera *out);

#endif
