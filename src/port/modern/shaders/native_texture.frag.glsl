#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D materialTexture;

vec4 materialTexel(uvec2 texel) {
    return texelFetch(materialTexture, ivec2(texel & uvec2(255u)), 0);
}

void main() {
    vec2 pixel = uv * 256.0;
    uvec2 nearestPosition = uvec2(clamp(floor(pixel), 0.0, 255.0));
    vec4 texel = materialTexel(nearestPosition);
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
        vec4 sampleColor = materialTexel(uvec2(at));
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
