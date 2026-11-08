#version 450

layout(location = 0) in vec3 worldDirection;
layout(location = 1) in vec2 screenUV;
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
    vec4 atmosphere; // enabled, authored cloud profile, reserved, reserved
} sky;

float cloudHash(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

float cloudNoise(vec2 p) {
    vec2 cell = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(cloudHash(cell), cloudHash(cell + vec2(1, 0)), f.x),
               mix(cloudHash(cell + vec2(0, 1)),
                   cloudHash(cell + vec2(1, 1)), f.x), f.y);
}

vec3 atmosphericSky(vec3 direction) {
    float height = direction.y;
    vec3 color = mix(sky.horizon.rgb, sky.middle.rgb,
                     smoothstep(0.0, 0.20, max(height, 0.0)));
    color = mix(color, sky.top.rgb, smoothstep(0.20, 0.70, height));
    if (height < 0.0)
        return mix(sky.horizon.rgb, sky.bottom.rgb,
                   smoothstep(0.0, 0.15, -height));
    /* A horizontal layer sampled along a world-space ray: continuous across
     * yaw zero, independent of viewport, camera pitch/roll and translation.
     * Four scales break up the PS1 panorama's repeated tile silhouettes. */
    vec2 p = direction.xz / max(height + 0.16, 0.16) * 2.2;
    float cloud = 0.0, weight = 0.57;
    mat2 turn = mat2(0.8, -0.6, 0.6, 0.8);
    for (int i = 0; i < 4; ++i) {
        cloud += cloudNoise(p) * weight;
        p = turn * p * 2.07 + vec2(17.1, 9.2);
        weight *= 0.48;
    }
    float coverage = smoothstep(mix(0.59, 0.49, sky.atmosphere.y), 0.76, cloud);
    coverage *= smoothstep(0.015, 0.10, height);
    float daylight = clamp(dot(sky.horizon.rgb, vec3(0.2126, 0.7152, 0.0722)) * 2.0, 0.0, 1.0);
    vec3 cloudColor = mix(sky.middle.rgb, vec3(0.95, 0.96, 0.98), daylight * 0.85);
    cloudColor *= mix(0.68, 1.0, smoothstep(0.48, 0.80, cloud));
    /* Retain the selected environment/custom panorama's palette in the new
     * layer, without its screen-space tile density or four yaw repetitions. */
    /* Palette samples are constant across the layer. Mapping nearest-filtered
     * PS1 texels onto it would put large pixel blocks back into smooth clouds. */
    vec4 authored = (texture(panorama, vec2(0.125, 0.5)) +
                     texture(panorama, vec2(0.375, 0.5)) +
                     texture(panorama, vec2(0.625, 0.5)) +
                     texture(panorama, vec2(0.875, 0.5))) * 0.25;
    cloudColor = mix(cloudColor, authored.rgb * mix(0.10, 1.0, daylight), authored.a * 0.22);
    return mix(color, cloudColor, coverage);
}

void main() {
    /*
     * The ray already points the way the picture does: the visible sky sits
     * at positive height, and flipping it here put the whole of it in the
     * band meant for below the horizon, whatever the gradient said.
     */
    vec3 direction = normalize(worldDirection);
    if (sky.atmosphere.x > 0.5) {
        outColor = vec4(atmosphericSky(direction), 1.0);
        return;
    }
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
    /* Interpolate top-left screen coordinates from the fullscreen triangle.
     * Fragment-position Y has different conventions across GPU backends. */
    vec2 screenPixel = vec2(
        screenUV.x * sky.gridOrigin.w -
            (sky.gridOrigin.w - 320.0) * 0.5,
        screenUV.y * 240.0);
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
    float cloudBand = (columnAxis.x * relative.y -
                       columnAxis.y * relative.x) / determinant;
    float panoramaHeight = float(textureSize(panorama, 0).y);
    float panoramaV = fract(cloudBand);
    if (panoramaHeight > 128.0) {
        /* Classic starts row zero at the origin, then places rows 1..3 above
         * it by subtracting rowStep. Thus the visible grid coordinates are
         * -3..1, with the two authored map rows alternating upwards. */
        float row = mod(-floor(cloudBand), 2.0);
        panoramaV = (row + fract(cloudBand)) * 0.5;
    }
    /* The original advances through 32 tile columns over one camera turn,
     * while the packed panorama contains eight columns. Anchor those four
     * repeats in world direction so scripted and interpolated cameras cannot
     * make the clouds counter-scroll relative to the course. At yaw zero the
     * classic centre sample is 10.5 tiles into the eight-tile panorama. */
    const float tau = 6.283185307179586;
    float panoramaU = fract(atan(direction.x, -direction.z) *
                            (4.0 / tau) + 0.3125);
    vec2 panoramaUV = vec2(panoramaU, panoramaV);
    vec4 authored = texture(panorama, panoramaUV);
    /* The band ends where the sheet ends. Fading it out over a stretch of
     * sky instead makes the cloud look like it is dissolving, and the sheet
     * is already transparent at its top edge, so there is nothing to hide. */
    /* The classic grid continues above the viewport during pitched intro
     * cameras. Its texture alpha supplies the upper edge; an extra height
     * cutoff made a conspicuous horizontal end to the clouds. */
    /* The four 128-line classic rows are taller than the 240-line viewport.
     * Repeating their vertical mapping avoids exposing a hard sheet edge when
     * a scripted camera advances after the classic background pass. The
     * one-row horizon variant remains deliberately bounded. */
    float cloudCoverage = sky.gridParams.z == 1.0
                              ? step(0.0, cloudBand) *
                                    step(cloudBand, 1.0)
                              : 1.0;
    /* Course geometry and water belong below the camera horizon. The classic
     * renderer hides this part of the sheet with its ground skirt; the native
     * renderer must apply the equivalent mask before drawing real geometry. */
    cloudCoverage *= step(0.0, height) * validGrid;
    color = mix(color, authored.rgb,
                authored.a * sky.bottom.a * cloudCoverage);
    outColor = vec4(color, 1.0);
}
