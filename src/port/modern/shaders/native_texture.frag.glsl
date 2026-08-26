#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 6) in vec3 shadowCoord;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D materialTexture;
layout(set = 2, binding = 1) uniform sampler2D shadowMap;

const vec3 LIGHT_DIRECTION = normalize(vec3(-0.1, 1.0, 0.12));

float shadowVisibility(vec3 n) {
    if (shadowCoord.x <= 0.0 || shadowCoord.x >= 1.0 ||
        shadowCoord.y <= 0.0 || shadowCoord.y >= 1.0 ||
        shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0) return 1.0;
    float facing = max(dot(n, LIGHT_DIRECTION), 0.0);
    float bias = mix(0.00025, 0.00008, facing);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float visible = 0.0;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            float storedDepth = texture(
                shadowMap,
                shadowCoord.xy + (vec2(x, y) - 0.5) * texelSize).r;
            visible += shadowCoord.z - bias <= storedDepth ? 1.0 : 0.0;
        }
    }
    return visible * 0.25;
}

vec4 materialTexel(ivec2 texel) {
    ivec2 limit = textureSize(materialTexture, 0) - ivec2(1);
    return texelFetch(materialTexture, clamp(texel, ivec2(0), limit), 0);
}

void main() {
    vec2 imageSize = vec2(textureSize(materialTexture, 0));
    vec2 pixel = uv * imageSize;
    ivec2 nearestPosition = ivec2(clamp(floor(pixel), vec2(0.0),
                                          imageSize - 1.0));
    vec4 texel = materialTexel(nearestPosition);
    if (texel.a <= 0.001) discard;
    vec2 samplePosition = pixel - 0.5;
    vec2 cell = floor(samplePosition);
    vec2 fraction = samplePosition - cell;
    vec3 filtered = vec3(0.0);
    float weightSum = 0.0;
    for (int tap = 0; tap < 4; tap++) {
        vec2 offset = vec2(float(tap & 1), float(tap >> 1));
        vec2 at = clamp(cell + offset, vec2(0.0), imageSize - 1.0);
        vec2 axis = abs(offset - fraction);
        float weight = (1.0 - axis.x) * (1.0 - axis.y);
        vec4 sampleColor = materialTexel(ivec2(at));
        if (sampleColor.a > 0.001) {
            filtered += sampleColor.rgb * weight;
            weightSum += weight;
        }
    }
    if (weightSum > 0.0) texel.rgb = filtered / weightSum;
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, LIGHT_DIRECTION), 0.0);
    vec3 light = mix(vec3(1.0),
        environmentLight * (0.35 + 0.65 * diffuse), lighting);
    float shadow = mix(0.62, 1.0, shadowVisibility(n));
    light *= mix(shadow, 1.0, fog.a);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
    outColor = vec4(texel.rgb * modulation * light, texel.a * color.a);
}
