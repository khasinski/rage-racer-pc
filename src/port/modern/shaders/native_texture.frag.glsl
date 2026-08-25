#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D vram;
layout(set = 3, binding = 0, std140) uniform NativeMaterial {
    uvec4 source;
    uvec4 window;
} material;

uint rgb5551(vec4 value) {
    uvec4 rgba = uvec4(value * vec4(31.0, 31.0, 31.0, 1.0) + 0.5);
    return rgba.r | (rgba.g << 5) | (rgba.b << 10) | (rgba.a << 15);
}

vec4 liveTexel(uvec2 texel) {
    uint tpage = material.source.x;
    if (material.source.z != 0u) {
        return texelFetch(vram, ivec2(texel & uvec2(255u)), 0);
    }
    uint mode = (tpage >> 7) & 3u;
    uvec2 page = uvec2((tpage & 15u) * 64u,
                       ((tpage >> 4) & 1u) * 256u);
    uvec2 size = max(material.window.xy, uvec2(1u));
    texel = (texel % size) + material.window.zw;
    if (mode >= 2u) return texelFetch(vram, ivec2(page + texel), 0);
    uint shift = mode == 1u ? 1u : 2u;
    uint subMask = mode == 1u ? 1u : 3u;
    uint indexShift = mode == 1u ? 8u : 4u;
    uint indexMask = mode == 1u ? 255u : 15u;
    uint word = rgb5551(texelFetch(
        vram, ivec2(page + uvec2(texel.x >> shift, texel.y)), 0));
    uint index = (word >> ((texel.x & subMask) * indexShift)) & indexMask;
    uvec2 clut = uvec2((material.source.y & 63u) * 16u,
                       material.source.y >> 6);
    return texelFetch(vram, ivec2(clut + uvec2(index, 0u)), 0);
}

void main() {
    vec2 pixel = uv * 256.0;
    uvec2 nearestPosition = uvec2(clamp(floor(pixel), 0.0, 255.0));
    vec4 texel = liveTexel(nearestPosition);
    if (texel.a <= 0.001) discard;
    vec2 samplePosition = pixel - 0.5;
    vec2 cell = floor(samplePosition);
    vec2 fraction = samplePosition - cell;
    vec3 filtered = vec3(0.0);
    float weightSum = 0.0;
    for (int tap = 0; tap < 4; tap++) {
        vec2 offset = vec2(float(tap & 1), float(tap >> 1));
        vec2 at = clamp(cell + offset, 0.0, 255.0);
        vec2 axis = abs(offset - fraction);
        float weight = (1.0 - axis.x) * (1.0 - axis.y);
        vec4 sampleColor = liveTexel(uvec2(at));
        if (sampleColor.a > 0.001) {
            filtered += sampleColor.rgb * weight;
            weightSum += weight;
        }
    }
    if (weightSum > 0.0) texel.rgb = filtered / weightSum;
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(vec3(-0.4, 0.7, 0.5))), 0.0);
    vec3 light = mix(vec3(1.0),
        environmentLight * (0.35 + 0.65 * diffuse), lighting);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
    outColor = vec4(texel.rgb * modulation * light, texel.a * color.a);
}
