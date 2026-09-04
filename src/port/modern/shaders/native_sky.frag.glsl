#version 450

layout(location = 0) in vec3 worldDirection;
layout(location = 1) in vec2 screenPosition;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D panorama;

layout(set = 3, binding = 0, std140) uniform NativeSkyColors {
    vec4 top;
    vec4 middle;
    vec4 horizon;
    vec4 bottom;
    vec4 gridOrigin;
    vec4 gridBasis;
    vec4 gridParams;
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
        /*
         * Below the band the game draws a flat quad, not a gradient:
         * DrawSkyBackground puts down a POLY_F4 in slot 4, so every corner
         * carries the same colour. Fading from the skyline colour instead
         * left a pale strip under the horizon, which reads as sky wherever
         * the ground ends short of the skyline.
         */
        color = sky.bottom.rgb;
    }
    /*
     * The cloud sheet wraps the horizon as a band, the way the game's tile
     * grid does: sixteen columns round a full turn. Vertically, classic draws
     * four authored bands alternating the two map rows selected for this
     * skybox; the explicit mapping below reproduces that bounded layout.
     */
    /* Classic's clouds are a screen-space 64x128-pixel grid, not a sphere.
     * Reconstruct that grid in the 240-line logical viewport. This keeps its
     * authored pixel density, its tile boundary, pitch and roll together. */
    vec2 screenPixel = vec2((screenPosition.x + 1.0) *
                                sky.gridOrigin.w * 0.5,
                            (1.0 - screenPosition.y) * 120.0);
    vec2 gridStart = sky.gridParams.z == 1.0
                         ? sky.gridParams.xy
                         : sky.gridOrigin.xy;
    vec2 relative = screenPixel - gridStart;
    vec2 columnAxis = sky.gridBasis.xy;
    vec2 rowAxis = sky.gridBasis.zw;
    float determinant = columnAxis.x * rowAxis.y -
                        columnAxis.y * rowAxis.x;
    float validGrid = step(0.0001, abs(determinant));
    determinant = validGrid != 0.0 ? determinant : 1.0;
    float gridColumn = (relative.x * rowAxis.y -
                        relative.y * rowAxis.x) / determinant;
    float cloudBand = (columnAxis.x * relative.y -
                       columnAxis.y * relative.x) / determinant;
    float panoramaHeight = float(textureSize(panorama, 0).y);
    float panoramaV = fract(cloudBand);
    if (panoramaHeight > 128.0) {
        /* Classic draws four vertical bands and alternates the two map rows
         * selected by g_SkyRowBase. The 256-row native texture stores those
         * two panoramas one above the other. */
        float row = mod(floor(cloudBand), 2.0);
        panoramaV = (row + fract(cloudBand)) * 0.5;
    }
    vec2 panoramaUV = vec2(fract((sky.gridOrigin.z + gridColumn) / 8.0),
                            panoramaV);
    vec4 authored = texture(panorama, panoramaUV);
    /* The band ends where the sheet ends. Fading it out over a stretch of
     * sky instead makes the cloud look like it is dissolving, and the sheet
     * is already transparent at its top edge, so there is nothing to hide. */
    /* The classic grid continues above the viewport during pitched intro
     * cameras. Its texture alpha supplies the upper edge; an extra height
     * cutoff made a conspicuous horizontal end to the clouds. */
    float cloudCoverage = step(0.0, cloudBand) *
                          step(cloudBand, sky.gridParams.z) * validGrid;
    color = mix(color, authored.rgb,
                authored.a * sky.bottom.a * cloudCoverage);
    outColor = vec4(color, 1.0);
}
