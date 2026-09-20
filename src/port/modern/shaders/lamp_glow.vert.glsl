#version 450
layout(set = 1, binding = 0, std140) uniform Glow {
    vec4 center;
    vec4 radius;
} glow;
layout(location = 0) out vec2 uv;
void main() {
    const vec2 corners[6] = vec2[6](vec2(-1,-1), vec2(1,-1), vec2(-1,1),
                                  vec2(-1,1), vec2(1,-1), vec2(1,1));
    uv = corners[gl_VertexIndex];
    gl_Position = glow.center;
    gl_Position.xy += uv * glow.radius.xy;
}
