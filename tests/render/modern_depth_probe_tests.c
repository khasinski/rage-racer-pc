#include <assert.h>
#include <math.h>

#include "modern/modern_depth_probe.h"

int main(void) {
    ModernDepthProbeVertex crossing[3] = {
        {{-1.0f, 0.0f, -2.0f}, 2.0f},
        {{ 1.0f, 0.0f, -2.0f}, 4.0f},
        {{ 0.0f, 1.0f, -0.5f}, 8.0f},
    };
    ModernDepthProbeVertex clipped[4];
    ModernDepthProbeVertex behind[3] = {
        {{0.0f, 0.0f, -0.1f}, 0.0f},
        {{1.0f, 0.0f, -0.2f}, 0.0f},
        {{0.0f, 1.0f, -0.3f}, 0.0f},
    };
    ModernDepthProbeVertex triangle[3] = {
        {{-1.0f, -1.0f, -2.0f}, 0.0f},
        {{ 1.0f, -1.0f, -2.0f}, 0.0f},
        {{ 0.0f,  1.0f, -2.0f}, 0.0f},
    };
    float depth = 0.0f;
    assert(ModernDepthProbeClipNear(crossing, clipped, 1.0f) == 4);
    assert(fabsf(clipped[0].view.z + 1.0f) < 0.0001f);
    assert(fabsf(clipped[3].view.z + 1.0f) < 0.0001f);
    assert(ModernDepthProbeClipNear(behind, clipped, 1.0f) == 0);
    assert(ModernDepthProbeTriangle(triangle, 160.0f, 120.0f, 320, 240,
                                    4.0f / 3.0f, 1.0f, 1.0f, 0.0f, &depth));
    assert(fabsf(depth - 1.0f) < 0.0001f);
    assert(!ModernDepthProbeTriangle(triangle, 0.0f, 0.0f, 320, 240,
                                     4.0f / 3.0f, 1.0f, 1.0f, 0.0f, &depth));
    return 0;
}
