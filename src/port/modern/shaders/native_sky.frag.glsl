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
    float horizontalLength = max(length(direction.xz), 0.001);
    float verticalSlope = height / horizontalLength;
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
    /* The authored image is one 90-degree cylinder band. The original asset
     * alternated it with a half-turn offset in successive vertical bands.
     * Keep that semantic layout, but evaluate it from a world-space ray so
     * replay and mirror cameras can turn freely without exposing a clamped
     * texture edge. */
    float bandCoordinate = 1.0 - verticalSlope * 2.5;
    float band = floor(bandCoordinate);
    float bandOffset = mod(band, 2.0) * 0.5;
    vec2 panoramaUV = vec2(
        fract(atan(direction.z, direction.x) * 0.6366197724 + 0.25 +
              bandOffset),
        fract(bandCoordinate));
    vec4 authored = texture(panorama, panoramaUV);
    /* The reconstructed camera basis maps the visible upper half of the view
     * to positive worldDirection.y. Keep the authored cloud cylinder there;
     * the lower half must remain the horizon-to-bottom gradient. */
    float upperHemisphereCoverage = smoothstep(0.0, 0.08, verticalSlope);
    float cylinderCoverage = upperHemisphereCoverage *
        (1.0 - smoothstep(0.9, 1.25, verticalSlope));
    color = mix(color, authored.rgb,
                authored.a * sky.bottom.a * cylinderCoverage);
    outColor = vec4(color, 1.0);
}
