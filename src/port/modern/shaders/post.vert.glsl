#version 450
layout(location = 0) out vec2 uv;
void main() {
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
    uv = vec2(corner.x, 1.0 - corner.y);
}
