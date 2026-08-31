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
     * The cloud sheet is a flat layer overhead, so project the ray onto it
     * rather than wrapping the image round the sky: a band expressed in the
     * ray's height squeezes the whole sheet into a few rows of screen and
     * draws it as a streak. Distance along the sheet is 1/height, which is
     * what gives cloud its perspective towards the horizon.
     */
    float cloudReach = 1.0 / max(height, 0.001);
    vec2 panoramaUV = vec2(direction.x, direction.z) * cloudReach * 0.80;
    vec4 authored = texture(panorama, panoramaUV);
    /* Cloud sits in a layer, so it thins out towards straight overhead as
     * well as fading into the horizon haze. Without the upper limit the
     * sheet tiles across the whole sky and covers far more of it than the
     * game's does. */
    float cloudCoverage = smoothstep(0.06, 0.16, height) *
        (1.0 - smoothstep(0.40, 0.70, height));
    color = mix(color, authored.rgb,
                authored.a * sky.bottom.a * cloudCoverage);
    outColor = vec4(color, 1.0);
}
