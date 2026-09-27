// GLSL ES 3.0 ports of the native modern renderer's shaders
// (src/port/modern/shaders/native.vert.glsl and native_texture.frag.glsl).
// Kept: world-to-view camera rows, projection, depth-bias rule, CPU-reference
// fog, PS1 colour modulation, scene/zone lighting. Left out for this
// experiment: ray-traced/shadow-map visibility, clear-coat and specular,
// vehicle lamps and spot lights.

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
uniform vec4 uCameraPosition;
uniform vec4 uViewRow0;
uniform vec4 uViewRow1;
uniform vec4 uViewRow2;
uniform vec4 uProjection;
out vec2 vUv;
out vec4 vColor;
out vec3 vNormal;
out vec4 vFog;
out float vLighting;
out vec3 vEnvironmentLight;

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
}
`;

export const worldFragment = /* glsl */ `
precision highp float;
uniform sampler2D uMaterial;
uniform float uTextured;
uniform vec4 uLightDirection;
uniform vec4 uAmbient;
uniform vec4 uDiffuse;
in vec2 vUv;
in vec4 vColor;
in vec3 vNormal;
in vec4 vFog;
in float vLighting;
in vec3 vEnvironmentLight;
out vec4 outColor;

void main() {
    vec3 n = dot(vNormal, vNormal) > 0.000001 ? normalize(vNormal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(uLightDirection.xyz)), 0.0);
    vec3 light = mix(vec3(1.0),
        vEnvironmentLight * (uAmbient.rgb + uDiffuse.rgb * diffuse), vLighting);
    light = mix(light, vec3(1.0), vFog.a);
    vec3 foggedColor = mix(vColor.rgb, vFog.rgb, vFog.a);
    if (uTextured > 0.5) {
        vec4 texel = texture(uMaterial, vUv);
        if (texel.a <= 0.001) discard;
        vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
        outColor = vec4(texel.rgb * modulation * light, texel.a * vColor.a);
    } else {
        outColor = vec4(foggedColor * light, vColor.a);
    }
}
`;

// Sky: the same three-band gradient the native shader reflects, evaluated
// per pixel along the camera ray.
export const skyVertex = /* glsl */ `
precision highp float;
in vec3 position;
out vec2 vNdc;
void main() {
    vNdc = position.xy;
    gl_Position = vec4(position.xy, 1.0, 1.0);
}
`;

export const skyFragment = /* glsl */ `
precision highp float;
uniform vec4 uViewRow0;
uniform vec4 uViewRow1;
uniform vec4 uViewRow2;
uniform vec4 uProjection;
uniform vec4 uSkyTop;
uniform vec4 uSkyHorizon;
uniform vec4 uSkyBottom;
in vec2 vNdc;
out vec4 outColor;
void main() {
    vec3 view = vec3(vNdc.x / uProjection.x, vNdc.y / uProjection.y, -1.0);
    vec3 direction = normalize(uViewRow0.xyz * view.x + uViewRow1.xyz * view.y + uViewRow2.xyz * view.z);
    vec3 color = direction.y >= 0.0
        ? mix(uSkyHorizon.rgb, uSkyTop.rgb, smoothstep(0.0, 0.8, direction.y))
        : mix(uSkyHorizon.rgb, uSkyBottom.rgb, smoothstep(0.0, 0.55, -direction.y));
    outColor = vec4(color, 1.0);
}
`;
