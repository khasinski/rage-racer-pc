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
    /*
     * The ray already points the way the picture does: the visible sky sits
     * at positive height, and flipping it here put the whole of it in the
     * band meant for below the horizon, whatever the gradient said.
     */
    vec3 direction = normalize(worldDirection);
    float height = direction.y;
    vec3 color;
    /*
     * Above the horizon the gradient runs from the horizon colour up through
     * the middle to the top. Below it the sky darkens to the bottom colour
     * quickly: the game's lower band is the dark one, and blending down
     * there towards the horizon instead left the bottom of the picture the
     * pale colour that belongs at the skyline.
     */
    if (height >= 0.0) {
        color = mix(sky.horizon.rgb, sky.middle.rgb,
                    smoothstep(0.0, 0.20, height));
        color = mix(color, sky.top.rgb,
                    smoothstep(0.20, 0.70, height));
    } else {
        color = mix(sky.horizon.rgb, sky.bottom.rgb,
                    smoothstep(0.0, 0.12, -height));
    }
    /*
     * The cloud sheet wraps the horizon as a band, the way the game's tile
     * grid does: sixteen columns round a full turn, two rows deep. It does
     * not repeat upwards. Tiling it in both directions instead, as a plane
     * overhead would, turns the sky into wallpaper, and no amount of scaling
     * hides that: the eye reads the repetition, not the cloud.
     */
    float cloudBand = clamp((0.535 - height) / 0.43, 0.0, 1.0);
    vec2 panoramaUV = vec2(
        fract(atan(direction.z, direction.x) * 0.31830989),
        cloudBand);
    vec4 authored = texture(panorama, panoramaUV);
    /* The band ends where the sheet ends. Fading it out over a stretch of
     * sky instead makes the cloud look like it is dissolving, and the sheet
     * is already transparent at its top edge, so there is nothing to hide. */
    float cloudCoverage = smoothstep(0.09, 0.15, height) *
        (1.0 - smoothstep(0.528, 0.535, height));
    color = mix(color, authored.rgb,
                authored.a * sky.bottom.a * cloudCoverage);
    outColor = vec4(color, 1.0);
}
