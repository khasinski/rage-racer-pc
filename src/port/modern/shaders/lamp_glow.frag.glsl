#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;
layout(set = 3, binding = 0, std140) uniform GlowColor { vec4 color; } glow;
void main() {
    float r2 = dot(uv, uv);
    float halo = exp(-4.0 * r2) * (1.0 - smoothstep(0.3, 1.0, r2));
    outColor = vec4(glow.color.rgb * halo, 0.0);
}
