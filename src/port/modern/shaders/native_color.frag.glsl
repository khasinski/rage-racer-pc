#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 6) in vec3 shadowCoord;
layout(location = 7) in float shadowReception;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D shadowMap;

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

void main() {
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, LIGHT_DIRECTION), 0.0);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 light = mix(vec3(1.0),
        environmentLight * (0.35 + 0.65 * diffuse), lighting);
    float visibility = shadowReception > 0.5 ? shadowVisibility(n) : 1.0;
    float shadow = mix(0.62, 1.0, visibility);
    light *= mix(shadow, 1.0, fog.a);
    outColor = vec4(foggedColor * light, color.a);
}
