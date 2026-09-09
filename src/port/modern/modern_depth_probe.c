#include "modern_depth_probe.h"

#include <math.h>

uint32_t ModernDepthProbeClipNear(const ModernDepthProbeVertex input[3],
                                  ModernDepthProbeVertex output[4],
                                  float nearPlane) {
    uint32_t inputIndex, count = 0;
    ModernDepthProbeVertex previous = input[2];
    int previousInside = -previous.view.z >= nearPlane;
    for (inputIndex = 0; inputIndex < 3; inputIndex++) {
        ModernDepthProbeVertex current = input[inputIndex];
        int currentInside = -current.view.z >= nearPlane;
        if (currentInside != previousInside) {
            float boundaryZ = -nearPlane;
            float t = (boundaryZ - previous.view.z) /
                      (current.view.z - previous.view.z);
            ModernDepthProbeVertex clipped;
            clipped.view.x = previous.view.x + (current.view.x - previous.view.x) * t;
            clipped.view.y = previous.view.y + (current.view.y - previous.view.y) * t;
            clipped.view.z = boundaryZ;
            clipped.depthBias = previous.depthBias +
                                (current.depthBias - previous.depthBias) * t;
            output[count++] = clipped;
        }
        if (currentInside) output[count++] = current;
        previous = current;
        previousInside = currentInside;
    }
    return count;
}

int ModernDepthProbeTriangle(const ModernDepthProbeVertex triangle[3],
                             float probeX, float probeY, int width, int height,
                             float aspect, float fovScale, float depthScale,
                             float depthOffset, float *depthOut) {
    float screenX[3], screenY[3], screenDepth[3];
    float denominator, a, b, c;
    int corner;
    for (corner = 0; corner < 3; corner++) {
        float depth = -triangle[corner].view.z;
        float ndcX = triangle[corner].view.x * fovScale / (depth * aspect);
        float ndcY = triangle[corner].view.y * fovScale / depth;
        screenX[corner] = (ndcX + 1.0f) * 0.5f * (float)width;
        screenY[corner] = (1.0f - ndcY) * 0.5f * (float)height;
        screenDepth[corner] = depthScale + depthOffset / depth +
                              triangle[corner].depthBias / 1048576.0f;
    }
    denominator = (screenY[1] - screenY[2]) * (screenX[0] - screenX[2]) +
                  (screenX[2] - screenX[1]) * (screenY[0] - screenY[2]);
    if (fabsf(denominator) < 0.000001f) return 0;
    a = ((screenY[1] - screenY[2]) * (probeX - screenX[2]) +
         (screenX[2] - screenX[1]) * (probeY - screenY[2])) / denominator;
    b = ((screenY[2] - screenY[0]) * (probeX - screenX[2]) +
         (screenX[0] - screenX[2]) * (probeY - screenY[2])) / denominator;
    c = 1.0f - a - b;
    if (a < -0.00001f || b < -0.00001f || c < -0.00001f) return 0;
    *depthOut = a * screenDepth[0] + b * screenDepth[1] + c * screenDepth[2];
    return 1;
}
