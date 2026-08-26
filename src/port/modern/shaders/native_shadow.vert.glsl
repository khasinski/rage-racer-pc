#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 1, binding = 0, std140) uniform NativeShadowCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
} shadow;

void main() {
    vec3 relative = inPosition - shadow.position.xyz;
    float depth = -dot(shadow.viewRow2.xyz, relative);
    gl_Position = vec4(
        dot(shadow.viewRow0.xyz, relative) * shadow.projection.x,
        dot(shadow.viewRow1.xyz, relative) * shadow.projection.y,
        depth * shadow.projection.z + shadow.projection.w,
        1.0);
}
