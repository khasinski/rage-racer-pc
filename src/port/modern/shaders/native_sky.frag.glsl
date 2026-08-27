#version 450

layout(location = 0) in vec3 worldDirection;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D panorama;

layout(set = 3, binding = 0, std140) uniform NativeSkyColors {
    vec4 top;
    vec4 middle;
    vec4 horizon;
    vec4 bottom;
} sky;

void main() {
    vec3 direction = normalize(worldDirection);
    float height = direction.y;
    vec3 color;
    if (height >= 0.0) {
        color = mix(sky.middle.rgb, sky.top.rgb,
                    smoothstep(0.0, 0.65, height));
    } else if (height >= -0.18) {
        color = mix(sky.middle.rgb, sky.horizon.rgb,
                    smoothstep(0.0, 0.18, -height));
    } else {
        color = mix(sky.horizon.rgb, sky.bottom.rgb,
                    smoothstep(0.18, 0.65, -height));
    }
    vec2 panoramaUV = vec2(
        fract(atan(direction.z, direction.x) * 0.6366197724 + 0.25),
        clamp(1.0 - height * 2.2, 0.0, 1.0));
    vec4 authored = texture(panorama, panoramaUV);
    color = mix(color, authored.rgb, authored.a * sky.bottom.a);
    outColor = vec4(color, 1.0);
}
