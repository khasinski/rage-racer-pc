#version 450
#extension GL_GOOGLE_include_directive : require
#define LOCAL_BINDING 3
#include "native_local.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in uvec4 inColor;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inFog;
layout(location = 6) in float inDepthBias;

layout(set = 1, binding = 0, std140) uniform NativeCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
    vec4 fogColor;
    vec4 fogRange;
} camera;

layout(set = 1, binding = 1, std140) uniform NativeShadowCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
} shadow;

layout(set = 1, binding = 2, std140) uniform NativeInstance {
    vec4 environmentLight;
    vec4 properties; // lighting influence, shadow reception, scroll U, reserved
} instance;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec4 fog;
layout(location = 4) out float lighting;
layout(location = 5) out vec3 environmentLight;
layout(location = 6) out vec3 shadowCoord;
layout(location = 7) out float shadowReception;
layout(location = 8) out vec3 viewDirection;

void main() {
    vec3 worldPosition = inPosition;
    vec3 worldNormal = inNormal;
    vec4 worldFog = inFog;
    transformLocal(worldPosition, worldNormal, worldFog);
    vec3 relative = worldPosition - camera.position.xyz;
    vec3 view = vec3(dot(camera.viewRow0.xyz, relative),
                     dot(camera.viewRow1.xyz, relative),
                     dot(camera.viewRow2.xyz, relative));
    float viewDepth = -view.z;
    float clipDepth = viewDepth * camera.projection.z + camera.projection.w;
    // A constant normalized-depth bias grows quadratically in world units.
    // Keep the existing near-camera separation, but cap each authored step
    // at a quarter world unit so distant decals cannot jump behind scenery.
    float legacyBias = (inDepthBias / 1048576.0) * viewDepth;
    float worldBias = inDepthBias * 0.25;
    float boundedBias = -camera.projection.w * worldBias /
                        max(viewDepth + worldBias, 1.0);
    clipDepth += sign(legacyBias) * min(abs(legacyBias), abs(boundedBias));
    gl_Position = vec4(view.x * camera.projection.x,
                       view.y * camera.projection.y,
                       clipDepth,
                       viewDepth);
    uv = inUV + vec2(instance.properties.z, 0.0);
    color = vec4(inColor) / 255.0;
    normal = worldNormal;
    // Opt-in diagnostic path retains the CPU reference for identical-scene A/B.
    if (camera.fogColor.w > 0.0) {
        fog = worldFog;
    } else {
        // Keep fog tied to the original position, not camera-facing decal lift.
        float fogDepth = -dot(camera.viewRow2.xyz, worldFog.xyz - camera.position.xyz);
        float fogWeight = 0.0;
        if (worldFog.w > 0.0 && camera.fogRange.x > 0.0 &&
            !isnan(fogDepth) && !isinf(fogDepth)) {
            if (fogDepth >= camera.fogRange.y) fogWeight = 1.0;
            else if (fogDepth > camera.fogRange.x)
                fogWeight = clamp((camera.fogRange.z - 1.0 / fogDepth) /
                                  camera.fogRange.w, 0.0, 1.0);
        }
        fog = vec4(camera.fogColor.xyz, fogWeight);
    }
    lighting = instance.properties.x;
    environmentLight = instance.environmentLight.xyz;
    vec3 shadowRelative = worldPosition - shadow.position.xyz;
    float shadowX = dot(shadow.viewRow0.xyz, shadowRelative) *
                    shadow.projection.x;
    float shadowY = dot(shadow.viewRow1.xyz, shadowRelative) *
                    shadow.projection.y;
    float shadowDepth = -dot(shadow.viewRow2.xyz, shadowRelative);
    shadowCoord = vec3(shadowX * 0.5 + 0.5,
                       0.5 - shadowY * 0.5,
                       shadowDepth * shadow.projection.z +
                           shadow.projection.w);
    shadowReception = instance.properties.y;
    viewDirection = camera.position.xyz - worldPosition;
}
