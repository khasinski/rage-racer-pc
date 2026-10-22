#version 450
#extension GL_GOOGLE_include_directive : require
layout(set = 2, binding = 0) uniform sampler2D occlusionMap;
layout(set = 2, binding = 1) uniform sampler2D depthMap;
#include "ssao_common.glsl"
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

// One axis of a depth-aware blur. Taps on a different surface than the
// centre (a large depth difference) are dropped, so occlusion never bleeds
// across an edge. The vertical pass is drawn straight onto the scene with a
// multiplying blend, which is why this writes plain grey.
void main() {
    float centreDepth = linearDepth(texture(depthMap, uv).r);
    float total = texture(occlusionMap, uv).r * 0.2270270270;
    float weightSum = 0.2270270270;
    const float offsets[4] = float[4](1.3846153846, 3.2307692308, 5.0769230769, 6.9230769231);
    const float weights[4] = float[4](0.3162162162, 0.0702702703, 0.0162162162, 0.0027027027);
    if (texture(depthMap, uv).r >= SKY_DEPTH) {
        fragColor = vec4(1.0);
        return;
    }
    for (int i = 0; i < 4; i++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            vec2 st = uv + ssao.screen.zw * (offsets[i] * float(sign));
            float d = texture(depthMap, st).r;
            if (d >= SKY_DEPTH) continue;
            float similarity = exp(-abs(linearDepth(d) - centreDepth) /
                                   (0.015 * centreDepth + 1.0));
            float w = weights[i] * similarity;
            total += texture(occlusionMap, st).r * w;
            weightSum += w;
        }
    }
    float ao = total / weightSum;
    fragColor = vec4(vec3(ao), 1.0);
}
