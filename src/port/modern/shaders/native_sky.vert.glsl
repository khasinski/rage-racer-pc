#version 450

layout(set = 1, binding = 0, std140) uniform NativeCamera {
    vec4 position;
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 projection;
} camera;

layout(location = 0) out vec3 worldDirection;

void main() {
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec2 clip = corner * 2.0 - 1.0;
    vec3 viewDirection = vec3(clip.x / camera.projection.x,
                              clip.y / camera.projection.y, -1.0);
    worldDirection = camera.viewRow0.xyz * viewDirection.x +
                     camera.viewRow1.xyz * viewDirection.y +
                     camera.viewRow2.xyz * viewDirection.z;
    gl_Position = vec4(clip, 1.0, 1.0);
}
