#version 450
layout(location = 0) in vec2 uv;
layout(set = 2, binding = 0) uniform sampler2D frame;
layout(location = 0) out vec4 fragColor;

// Vibrant grading, matching the original Metal-only composite pass.
// The renderer omits this pass entirely when grading is disabled.
void main() {
    vec3 c = texture(frame, uv).rgb;
    float luma = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(luma), c, 1.16);
    c = (c - 0.5) * 1.04 + 0.5;
    fragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
