#ifndef RAGE_MODERN_DEPTH_PROBE_H
#define RAGE_MODERN_DEPTH_PROBE_H

#include "render/render_world.h"

#include <stdint.h>

typedef struct ModernDepthProbeVertex {
    RageRenderVec3 view;
    float depthBias;
} ModernDepthProbeVertex;

uint32_t ModernDepthProbeClipNear(const ModernDepthProbeVertex input[3],
                                  ModernDepthProbeVertex output[4],
                                  float nearPlane);
int ModernDepthProbeTriangle(const ModernDepthProbeVertex triangle[3],
                             float probeX, float probeY, int width, int height,
                             float aspect, float fovScale, float depthScale,
                             float depthOffset, float *depthOut);

#endif
