#version 450
#extension GL_GOOGLE_include_directive : require
#define LOCAL_BINDING 2
#include "native_local.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec2 uv;

layout(set = 1, binding = 0, std140) uniform NativeShadowCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
} shadow;

layout(set = 1, binding = 1, std140) uniform NativeShadowInstance {
    vec4 uvOffset;
} instance;

void main() {
    vec3 worldPosition = inPosition;
    vec3 normal = inNormal;
    vec4 fog = vec4(0.0);
    transformLocal(worldPosition, normal, fog);
    vec3 relative = worldPosition - shadow.position.xyz;
    float depth = -dot(shadow.viewRow2.xyz, relative);
    gl_Position = vec4(
        dot(shadow.viewRow0.xyz, relative) * shadow.projection.x,
        dot(shadow.viewRow1.xyz, relative) * shadow.projection.y,
        depth * shadow.projection.z + shadow.projection.w,
        1.0);
    uv = inUV + instance.uvOffset.xy;
}
