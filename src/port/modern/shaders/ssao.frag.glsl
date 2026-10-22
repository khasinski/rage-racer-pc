#version 450
#extension GL_GOOGLE_include_directive : require
layout(set = 2, binding = 0) uniform sampler2D depthMap;
#include "ssao_common.glsl"
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

// Hemisphere ambient occlusion in view space. Sixteen taps follow a
// golden-angle spiral whose phase is rotated per pixel by interleaved
// gradient noise; the blur pass that follows removes the resulting grain
// without touching depth edges.
const int TAPS = 16;

float gradientNoise(vec2 pixel) {
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

float depthAt(vec2 st) {
    float d = texture(depthMap, st).r;
    return d >= SKY_DEPTH ? -1.0 : linearDepth(d);
}

void main() {
    float centreDepth = depthAt(uv);
    if (centreDepth < 0.0) {
        fragColor = vec4(1.0);
        return;
    }
    vec3 centre = viewPosition(uv, centreDepth);

    // Normal from the smaller depth step on each axis, so a silhouette edge
    // does not bend it.
    vec2 texel = ssao.screen.xy;
    float right = depthAt(uv + vec2(texel.x, 0.0));
    float left = depthAt(uv - vec2(texel.x, 0.0));
    float down = depthAt(uv + vec2(0.0, texel.y));
    float up = depthAt(uv - vec2(0.0, texel.y));
    if (right < 0.0) right = centreDepth;
    if (left < 0.0) left = centreDepth;
    if (down < 0.0) down = centreDepth;
    if (up < 0.0) up = centreDepth;
    vec3 dx = abs(right - centreDepth) < abs(centreDepth - left)
        ? viewPosition(uv + vec2(texel.x, 0.0), right) - centre
        : centre - viewPosition(uv - vec2(texel.x, 0.0), left);
    vec3 dy = abs(down - centreDepth) < abs(centreDepth - up)
        ? viewPosition(uv + vec2(0.0, texel.y), down) - centre
        : centre - viewPosition(uv - vec2(0.0, texel.y), up);
    vec3 normal = normalize(cross(dy, dx));
    if (dot(normal, centre) > 0.0) normal = -normal;

    vec3 helper = abs(normal.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    float radius = ssao.sampling.x;
    float phase = gradientNoise(gl_FragCoord.xy) * 6.2831853;
    float occlusion = 0.0;
    for (int i = 0; i < TAPS; i++) {
        float t = (float(i) + 0.5) / float(TAPS);
        float angle = phase + float(i) * 2.3999632;
        float ring = sqrt(t);
        vec3 direction = tangent * (cos(angle) * ring) +
                         bitangent * (sin(angle) * ring) +
                         normal * sqrt(1.0 - t);
        // Short taps are denser: contact shadow matters more than reach.
        float reach = mix(0.15, 1.0, t * t) * radius;
        vec3 samplePoint = centre + direction * reach;
        float sampleDepth = -samplePoint.z;
        if (sampleDepth <= 0.0) continue;
        vec2 ndc = samplePoint.xy * ssao.projection.xy / sampleDepth;
        vec2 st = vec2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
        if (st.x < 0.0 || st.x > 1.0 || st.y < 0.0 || st.y > 1.0) continue;
        float seen = depthAt(st);
        if (seen < 0.0) continue;
        // Occluded when visible geometry lies in front of the tap; geometry
        // far in front is a different surface and counts for less.
        float gap = sampleDepth - seen;
        float bias = 0.02 * radius + 0.0005 * centreDepth;
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(centreDepth - seen), 1e-4));
        occlusion += (gap > bias ? 1.0 : 0.0) * rangeCheck;
    }
    float visibility = 1.0 - occlusion / float(TAPS);
    visibility = pow(clamp(visibility, 0.0, 1.0), ssao.sampling.y);
    // Fog and haze carry no contact shadow; fade the effect out with range.
    float fade = 1.0 - smoothstep(ssao.sampling.z, ssao.sampling.w, centreDepth);
    fragColor = vec4(vec3(mix(1.0, visibility, fade)), 1.0);
}
