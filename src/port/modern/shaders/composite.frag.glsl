#version 450
layout(location = 0) in vec2 uv;
layout(set = 2, binding = 0) uniform sampler2D frame;
layout(location = 0) out vec4 fragColor;
layout(std140, set = 3, binding = 0) uniform Composite {
    vec4 controls; // brightness, grading, unused, unused
};

// Vibrant grading, matching the original Metal-only composite pass.
// Also applies the modern renderer brightness control independently of grading.
void main() {
    vec3 c = texture(frame, uv).rgb;
    if (controls.y > 0.5) {
        float luma = dot(c, vec3(0.299, 0.587, 0.114));
        c = mix(vec3(luma), c, 1.16);
        c = (c - 0.5) * 1.04 + 0.5;
    }
    // Lift midtones while preserving black and white. Classic uses 1.0.
    if (abs(controls.x - 1.0) > 0.0001)
        c = pow(clamp(c, 0.0, 1.0), vec3(1.0 / max(controls.x, 0.5)));
    fragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
