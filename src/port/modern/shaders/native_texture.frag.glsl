#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 7) in float shadowReception;
layout(location = 8) in vec3 viewDirection;
layout(location = 9) in vec3 worldPositionIn;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D materialTexture;
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;
#include "native_scene_light.glsl"
#include "native_spot.glsl"
#include "native_sun_shadow.glsl"
struct LampPatch {
    vec4 bounds;
    vec4 emission;
};
layout(set = 3, binding = 1, std140) uniform NativeMaterial {
    vec4 baseColor;
    vec4 emissiveAndShading;
    vec4 surface;
    LampPatch lamps[8];
} material;

vec3 reflectedSky(vec3 direction) {
    if (direction.y >= 0.0)
        return mix(sceneLight.skyHorizon.rgb, sceneLight.skyTop.rgb,
                   smoothstep(0.0, 0.8, direction.y));
    return mix(sceneLight.skyHorizon.rgb, sceneLight.skyBottom.rgb,
               smoothstep(0.0, 0.55, -direction.y));
}

void main() {
    /* Material mips are stored premultiplied so transparent atlas texels do
     * not contribute a black fringe. Convert back after filtered sampling. */
    vec4 texel = texture(materialTexture, uv);
    if ((material.surface.z > 1.5 && material.surface.z < 2.5 &&
         texel.a < 0.5) || texel.a <= 0.001) discard;
    texel.rgb /= texel.a;
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(sceneLight.direction.xyz)), 0.0);
    vec3 view = normalize(viewDirection);
    float materialLighting = lighting;
    if (material.emissiveAndShading.w >= 0.0)
        materialLighting = material.emissiveAndShading.w;
    float visibility = 1.0;
    if (shadowReception > 0.5 && materialLighting > 0.0 && fog.a < 1.0) {
        visibility = sunVisibility(worldPositionIn, n);
    }
    float shadow = mix(0.10, 1.0, visibility);
    float ambientShadow = mix(0.55, 1.0, visibility);
    vec3 light = mix(vec3(1.0),
        environmentLight * (sceneLight.ambient.rgb * ambientShadow +
            sceneLight.diffuse.rgb * diffuse * shadow),
        materialLighting);
    /* Authored vertex colours keep some baked brightness even on lit terrain,
     * so a shadow that only removes the sun term looks washed out. Darken the
     * blended result as well. */
    light *= mix(1.0, mix(0.60, 1.0, visibility), materialLighting);
    // Preserve texture contrast, but keep indirect light readable in tunnels.
    light = max(light, vec3(0.18));
    light = mix(light, vec3(1.0), fog.a);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
    vec3 base = texel.rgb * modulation * light * material.baseColor.rgb;
    float roughness = clamp(material.surface.x, 0.0, 1.0);
    float metallic = clamp(material.surface.y, 0.0, 1.0);
    float gloss = 1.0 - roughness;
    float coat = smoothstep(0.18, 0.55, gloss) *
        min(material.surface.w, 1.0);
    vec3 lightDirection = normalize(sceneLight.direction.xyz);
    vec3 halfDirection = normalize(lightDirection + view);
    float ndl = max(dot(n, lightDirection), 0.0);
    float ndv = max(dot(n, view), 0.001);
    float ndh = max(dot(n, halfDirection), 0.0);
    float vdh = max(dot(view, halfDirection), 0.0);
    vec3 materialColor = texel.rgb * material.baseColor.rgb;
    vec3 f0 = mix(vec3(0.06), materialColor, metallic);
    vec3 fresnel = f0 + (vec3(1.0) - f0) * pow(1.0 - vdh, 5.0);
    float alpha = max(roughness * roughness, 0.025);
    float alpha2 = alpha * alpha;
    float distributionDenominator = ndh * ndh * (alpha2 - 1.0) + 1.0;
    float distribution = alpha2 /
        max(3.14159265 * distributionDenominator *
            distributionDenominator, 0.0001);
    float geometryK = (roughness + 1.0) * (roughness + 1.0) * 0.125;
    float geometryView = ndv / (ndv * (1.0 - geometryK) + geometryK);
    float geometryLight = ndl / (ndl * (1.0 - geometryK) + geometryK);
    vec3 directSpecular = sceneLight.diffuse.rgb * fresnel * distribution *
        geometryView * geometryLight * ndl / max(4.0 * ndv * ndl, 0.001);
    float rim = pow(1.0 - ndv, 5.0);
    float zoneReflection = smoothstep(0.30, 0.85,
        min(environmentLight.r, min(environmentLight.g, environmentLight.b)));
    float reflectionStrength = coat * zoneReflection * mix(0.03, 0.18, rim) *
        mix(0.85, 1.15, metallic);
    vec3 reflectionDirection = reflect(-view, n);
    vec3 reflected = reflectedSky(reflectionDirection);
    float reflectedLuminance = dot(reflected, vec3(0.2126, 0.7152, 0.0722));
    reflected = mix(vec3(reflectedLuminance), reflected, 0.65);
    base = mix(base, reflected, clamp(reflectionStrength, 0.0, 0.85) * (1.0 - fog.a));
    directSpecular *= coat * zoneReflection;
    vec3 specular = directSpecular * step(0.001, materialLighting);
    base += texel.rgb * modulation * material.baseColor.rgb *
        spotLighting(worldPositionIn, n) * (1.0 - fog.a);
    vec3 emissive = texel.rgb * material.emissiveAndShading.rgb;
    for (int i = 0; i < 8; ++i) {
        vec4 bounds = material.lamps[i].bounds;
        if (material.lamps[i].emission.w > 0.5) {
            vec2 lens = (uv - (bounds.xy + bounds.zw) * 0.5) /
                        max((bounds.zw - bounds.xy) * 0.5, vec2(0.00001));
            if (dot(lens, lens) > 1.0) continue;
        }
        if (all(greaterThanEqual(uv, bounds.xy)) &&
            all(lessThan(uv, bounds.zw)))
            emissive += material.lamps[i].emission.rgb;
    }
    outColor = vec4(base + (specular + emissive) * (1.0 - fog.a),
                    texel.a * color.a * material.baseColor.a);
}
