#version 450
#extension GL_GOOGLE_include_directive : require
#define RAY_NODE_BINDING 1
#define RAY_TRIANGLE_BINDING 2
#define RAY_INDEX_BINDING 3
#define RAY_INSTANCE_BINDING 4
#include "native_ray.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 6) in vec3 shadowCoord;
layout(location = 7) in float shadowReception;
layout(location = 9) in vec3 worldPositionIn;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D shadowMap;
struct SpotLight {
    vec4 positionRange;
    vec4 directionOuter;
    vec4 colorInner;
};
layout(set = 3, binding = 0, std140) uniform NativeSceneLight {

    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 skyTop;
    vec4 skyHorizon;
    vec4 skyBottom;
    vec4 ray;
    vec4 spotCount;
    SpotLight spots[72];
} sceneLight;
#include "native_spot.glsl"

float shadowVisibility(vec3 n) {
    if (shadowCoord.x <= 0.0 || shadowCoord.x >= 1.0 ||
        shadowCoord.y <= 0.0 || shadowCoord.y >= 1.0 ||
        shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0) return 1.0;
    float facing = max(dot(n, normalize(sceneLight.direction.xyz)), 0.0);
    float bias = mix(0.00025, 0.00008, facing);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    vec2 pixel = shadowCoord.xy / texelSize - 0.5;
    vec2 fraction = fract(pixel);
    vec2 first = (floor(pixel) + 0.5) * texelSize;
    float visible = 0.0;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            float storedDepth = texture(
                shadowMap,
                first + vec2(x, y) * texelSize).r;
            float weight = (x == 0 ? 1.0 - fraction.x : fraction.x) *
                           (y == 0 ? 1.0 - fraction.y : fraction.y);
            visible += shadowCoord.z - bias <= storedDepth ? weight : 0.0;
        }
    }
    return visible;
}

void main() {
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(sceneLight.direction.xyz)), 0.0);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    float visibility = 1.0;
    if (shadowReception > 0.5) {
        if (sceneLight.ray.x > 0.5) {
            vec3 rayDirection = normalize(sceneLight.direction.xyz);
            float epsilon = max(0.02, length(worldPositionIn) * 0.000001);
            visibility = tracedVisibility(
                worldPositionIn + rayDirection * epsilon, rayDirection,
                uint(sceneLight.ray.y + 0.5), uint(sceneLight.ray.w + 0.5));
        } else {
            visibility = shadowVisibility(n);
        }
    }
    float shadow = mix(0.10, 1.0, visibility);
    float ambientShadow = mix(0.30, 1.0, visibility);
    vec3 light = mix(vec3(1.0),
        environmentLight * (sceneLight.ambient.rgb * ambientShadow +
            sceneLight.diffuse.rgb * diffuse * shadow),
        lighting);
    float tracedOcclusion = mix(0.35, 1.0, visibility);
    light *= mix(1.0, tracedOcclusion, lighting);
    outColor = vec4(foggedColor * light + color.rgb *
        spotLighting(worldPositionIn, n) * (1.0 - fog.a), color.a);
}
