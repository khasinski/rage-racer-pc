// GLSL ES 3.0 ports of the native modern renderer's shaders
// (src/port/modern/shaders/native.vert.glsl, native_texture.frag.glsl,
// native_color.frag.glsl, native_shadow*.glsl and native_sky.*.glsl). Kept:
// world-to-view camera rows, projection, depth-bias rule, CPU-reference fog,
// PS1 colour modulation, zone lighting, the premultiplied atlas mip chain,
// the vehicle shadow map with its 2x2 filter and native shading weights, and
// the sky gradient with the retail cloud panorama grid. Left
// out: ray-traced visibility, clear coat/specular, lamps and spot lights.

/* WebGL cannot limit a texture to four mip levels, so the chain is padded
 * and the level of detail clamped here to the native RAGE_TEXTURE_ATLAS_MIP_LEVELS. */
const atlasSample = /* glsl */ `
vec4 atlasTexel(sampler2D image, vec2 uv) {
    vec2 texels = uv * 256.0;
    vec2 dx = dFdx(texels), dy = dFdy(texels);
    float lod = 0.5 * log2(max(max(dot(dx, dx), dot(dy, dy)), 1e-8));
    return textureLod(image, uv, clamp(lod, 0.0, 3.0));
}
`;

export const worldVertex = /* glsl */ `
precision highp float;
in vec3 position;
in vec2 uv;
in vec4 color;
in vec3 normal;
in vec4 fog;
in float lighting;
in vec3 environmentLight;
in float depthBias;
in float shadowReception;
uniform vec4 uCameraPosition;
uniform vec4 uViewRow0;
uniform vec4 uViewRow1;
uniform vec4 uViewRow2;
uniform vec4 uProjection;
uniform vec4 uShadowPosition;
uniform vec4 uShadowRow0;
uniform vec4 uShadowRow1;
uniform vec4 uShadowRow2;
uniform vec4 uShadowProjection;
out vec2 vUv;
out vec4 vColor;
out vec3 vNormal;
out vec4 vFog;
out float vLighting;
out vec3 vEnvironmentLight;
out vec3 vShadowCoord;
out float vShadowReception;

void main() {
    vec3 relative = position - uCameraPosition.xyz;
    vec3 view = vec3(dot(uViewRow0.xyz, relative),
                     dot(uViewRow1.xyz, relative),
                     dot(uViewRow2.xyz, relative));
    float viewDepth = -view.z;
    float clipDepth = viewDepth * uProjection.z + uProjection.w;
    float legacyBias = (depthBias / 1048576.0) * viewDepth;
    float worldBias = depthBias * 0.25;
    float boundedBias = -uProjection.w * worldBias / max(viewDepth + worldBias, 1.0);
    clipDepth += sign(legacyBias) * min(abs(legacyBias), abs(boundedBias));
    // Native depth is 0..w (Metal/Vulkan); WebGL expects -w..w.
    gl_Position = vec4(view.x * uProjection.x, view.y * uProjection.y,
                       2.0 * clipDepth - viewDepth, viewDepth);
    vUv = uv;
    vColor = color / 255.0;
    vNormal = normal;
    vFog = fog;
    vLighting = lighting;
    vEnvironmentLight = environmentLight;
    vec3 shadowRelative = position - uShadowPosition.xyz;
    float shadowX = dot(uShadowRow0.xyz, shadowRelative) * uShadowProjection.x;
    float shadowY = dot(uShadowRow1.xyz, shadowRelative) * uShadowProjection.y;
    float shadowDepth = -dot(uShadowRow2.xyz, shadowRelative);
    // WebGL render targets put NDC y = -1 at texture row 0.
    vShadowCoord = vec3(shadowX * 0.5 + 0.5, shadowY * 0.5 + 0.5,
                        shadowDepth * uShadowProjection.z + uShadowProjection.w);
    vShadowReception = shadowReception;
}
`;

export const worldFragment = /* glsl */ `
precision highp float;
precision highp sampler2D;
uniform sampler2D uMaterial;
uniform sampler2D uShadowMap;
uniform float uTextured;
uniform float uShadowEnabled;
uniform float uShadowResolution;
uniform vec4 uLightDirection;
uniform vec4 uAmbient;
uniform vec4 uDiffuse;
in vec2 vUv;
in vec4 vColor;
in vec3 vNormal;
in vec4 vFog;
in float vLighting;
in vec3 vEnvironmentLight;
in vec3 vShadowCoord;
in float vShadowReception;
out vec4 outColor;
${atlasSample}
float shadowVisibility(vec3 n) {
    if (vShadowCoord.x <= 0.0 || vShadowCoord.x >= 1.0 ||
        vShadowCoord.y <= 0.0 || vShadowCoord.y >= 1.0 ||
        vShadowCoord.z <= 0.0 || vShadowCoord.z >= 1.0) return 1.0;
    float facing = max(dot(n, normalize(uLightDirection.xyz)), 0.0);
    float bias = mix(0.00025, 0.00008, facing);
    vec2 texelSize = vec2(1.0 / uShadowResolution);
    vec2 pixel = vShadowCoord.xy / texelSize - 0.5;
    vec2 fraction = fract(pixel);
    vec2 first = (floor(pixel) + 0.5) * texelSize;
    float visible = 0.0;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            float storedDepth = texture(uShadowMap, first + vec2(x, y) * texelSize).r;
            float weight = (x == 0 ? 1.0 - fraction.x : fraction.x) *
                           (y == 0 ? 1.0 - fraction.y : fraction.y);
            visible += vShadowCoord.z - bias <= storedDepth ? weight : 0.0;
        }
    }
    return visible;
}

void main() {
    vec3 n = dot(vNormal, vNormal) > 0.000001 ? normalize(vNormal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(uLightDirection.xyz)), 0.0);
    float visibility = uShadowEnabled > 0.5 && vShadowReception > 0.5 ? shadowVisibility(n) : 1.0;
    float shadow = mix(0.10, 1.0, visibility);
    float ambientShadow = mix(0.30, 1.0, visibility);
    vec3 light = mix(vec3(1.0),
        vEnvironmentLight * (uAmbient.rgb * ambientShadow + uDiffuse.rgb * diffuse * shadow),
        vLighting);
    light *= mix(1.0, mix(0.35, 1.0, visibility), vLighting);
    light = mix(light, vec3(1.0), vFog.a);
    vec3 foggedColor = mix(vColor.rgb, vFog.rgb, vFog.a);
    if (uTextured > 0.5) {
        vec4 texel = atlasTexel(uMaterial, vUv);
        if (texel.a <= 0.001) discard;
        texel.rgb /= texel.a;
        vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
        outColor = vec4(texel.rgb * modulation * light, texel.a * vColor.a);
    } else {
        outColor = vec4(foggedColor * light, vColor.a);
    }
}
`;

// Vehicle shadow map (native_shadow.vert.glsl / native_shadow_masked.frag.glsl).
export const shadowVertex = /* glsl */ `
precision highp float;
in vec3 position;
in vec2 uv;
uniform vec4 uShadowPosition;
uniform vec4 uShadowRow0;
uniform vec4 uShadowRow1;
uniform vec4 uShadowRow2;
uniform vec4 uShadowProjection;
out vec2 vUv;
void main() {
    vec3 relative = position - uShadowPosition.xyz;
    float depth = -dot(uShadowRow2.xyz, relative);
    gl_Position = vec4(dot(uShadowRow0.xyz, relative) * uShadowProjection.x,
                       dot(uShadowRow1.xyz, relative) * uShadowProjection.y,
                       2.0 * (depth * uShadowProjection.z + uShadowProjection.w) - 1.0,
                       1.0);
    vUv = uv;
}
`;

export const shadowFragment = /* glsl */ `
precision highp float;
uniform sampler2D uMaterial;
uniform float uTextured;
in vec2 vUv;
out vec4 outColor;
${atlasSample}
void main() {
    if (uTextured > 0.5 && atlasTexel(uMaterial, vUv).a <= 0.5) discard;
    outColor = vec4(0.0);
}
`;

// Sky (native_sky.vert.glsl / native_sky.frag.glsl): the environment
// gradient along the camera ray, with the disc's cloud panorama placed on
// the retail screen-space tile grid. One full-screen triangle, as natively.
export const skyVertex = /* glsl */ `
precision highp float;
in vec3 position;
uniform vec4 uViewRow0;
uniform vec4 uViewRow1;
uniform vec4 uViewRow2;
uniform vec4 uProjection;
out vec3 vWorldDirection;
void main() {
    vec2 clip = position.xy;
    vec3 viewDirection = vec3(clip.x / uProjection.x, clip.y / uProjection.y, -1.0);
    vWorldDirection = uViewRow0.xyz * viewDirection.x +
                      uViewRow1.xyz * viewDirection.y +
                      uViewRow2.xyz * viewDirection.z;
    gl_Position = vec4(clip, 1.0, 1.0);
}
`;

export const skyFragment = /* glsl */ `
precision highp float;
precision highp sampler2D;
uniform sampler2D uPanorama;
uniform vec4 uSkyTop;
uniform vec4 uSkyMiddle;
uniform vec4 uSkyHorizon;
uniform vec4 uSkyBottom;
uniform vec4 uSkyGridOrigin;
uniform vec4 uSkyGridBasis;
uniform vec4 uSkyGridParams;
in vec3 vWorldDirection;
out vec4 outColor;
void main() {
    vec3 direction = normalize(vWorldDirection);
    float height = direction.y;
    vec3 color;
    // Above the horizon: horizon, through the middle, to the top colour.
    // Below it the game draws a flat POLY_F4 in the bottom colour.
    if (height >= 0.0) {
        color = mix(uSkyHorizon.rgb, uSkyMiddle.rgb, smoothstep(0.0, 0.20, height));
        color = mix(color, uSkyTop.rgb, smoothstep(0.20, 0.70, height));
    } else {
        color = uSkyBottom.rgb;
    }
    // The retail 64x128-pixel cloud grid in the 240-line logical viewport,
    // from the GPU's bottom-left fragment coordinates to the PS1's top-left.
    vec2 screenPixel = vec2(
        gl_FragCoord.x * (240.0 / uSkyGridParams.w) - (uSkyGridOrigin.w - 320.0) * 0.5,
        (uSkyGridParams.w - gl_FragCoord.y) * (240.0 / uSkyGridParams.w));
    vec2 gridStart = uSkyGridParams.z == 1.0 ? uSkyGridParams.xy : uSkyGridOrigin.xy;
    vec2 relative = screenPixel - gridStart;
    vec2 columnAxis = uSkyGridBasis.xy;
    vec2 rowAxis = uSkyGridBasis.zw;
    float determinant = columnAxis.x * rowAxis.y - columnAxis.y * rowAxis.x;
    float validGrid = step(0.0001, abs(determinant));
    determinant = validGrid != 0.0 ? determinant : 1.0;
    float cloudBand = (columnAxis.x * relative.y - columnAxis.y * relative.x) / determinant;
    float panoramaHeight = float(textureSize(uPanorama, 0).y);
    float panoramaV = fract(cloudBand);
    if (panoramaHeight > 128.0) {
        // Rows -3..1 above the origin alternate the two authored map rows.
        float row = mod(-floor(cloudBand), 2.0);
        panoramaV = (row + fract(cloudBand)) * 0.5;
    }
    // 32 tile columns per turn over an eight-column panorama, anchored to
    // the world direction; at yaw zero the centre is 10.5 tiles in.
    const float tau = 6.283185307179586;
    float panoramaU = fract(atan(direction.x, -direction.z) * (4.0 / tau) + 0.3125);
    vec4 authored = texture(uPanorama, vec2(panoramaU, panoramaV));
    // The one-row horizon strip is bounded; the four-row sheet repeats.
    float cloudCoverage = uSkyGridParams.z == 1.0
        ? step(0.0, cloudBand) * step(cloudBand, 1.0) : 1.0;
    // Course geometry belongs below the camera horizon.
    cloudCoverage *= step(0.0, height) * validGrid;
    color = mix(color, authored.rgb, authored.a * uSkyBottom.a * cloudCoverage);
    outColor = vec4(color, 1.0);
}
`;

// modern_renderer.c ModernCompositeNativeMirror: the rear camera's picture
// blitted into the mirror panel flipped left to right, as a real mirror is.
export const mirrorVertex = /* glsl */ `
precision highp float;
in vec3 position;
in vec2 uv;
out vec2 vUv;
void main() {
    vUv = vec2(1.0 - uv.x, uv.y);
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
`;

export const mirrorFragment = /* glsl */ `
precision highp float;
uniform sampler2D uMirror;
in vec2 vUv;
out vec4 outColor;
void main() {
    outColor = vec4(texture(uMirror, vUv).rgb, 1.0);
}
`;

