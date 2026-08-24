#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in uvec4 inColor;
layout(location = 3) in vec3 inNormal;

layout(set = 1, binding = 0, std140) uniform NativeCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
} camera;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;
layout(location = 2) out vec3 normal;

void main() {
    vec3 relative = inPosition - camera.position.xyz;
    vec3 view = vec3(dot(camera.viewRow0.xyz, relative),
                     dot(camera.viewRow1.xyz, relative),
                     dot(camera.viewRow2.xyz, relative));
    float depth = (view.z - camera.projection.z) * camera.projection.w;
    gl_Position = vec4(view.x * camera.projection.x,
                       view.y * camera.projection.y,
                       depth * view.z, view.z);
    uv = inUV;
    color = vec4(inColor) / 255.0;
    normal = inNormal;
}
