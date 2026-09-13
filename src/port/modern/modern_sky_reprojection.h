#ifndef RAGE_MODERN_SKY_REPROJECTION_H
#define RAGE_MODERN_SKY_REPROJECTION_H

#include "scene_capture.h"
#include "render/render_world.h"

/* Reproject captured sky points from the camera that emitted the packets to
 * the interpolated presentation camera. Neither pointer is retained. */
void ModernSkyReprojectPoint(const RenderCamera *source,
                             const RenderCamera *target,
                             float *x, float *y);
int ModernSkyBuildSmoothQuad(const RageCapturePacket *packet,
                             const RenderCamera *source,
                             const RenderCamera *target,
                             float x[4], float y[4]);

#endif
