#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 6) in vec3 shadowCoord;
layout(location = 7) in float shadowReception;
layout(location = 8) in vec3 viewDirection;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D materialTexture;
layout(set = 2, binding = 1) uniform sampler2D shadowMap;
layout(set = 3, binding = 0, std140) uniform NativeSceneLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 skyTop;
    vec4 skyHorizon;
    vec4 skyBottom;
} sceneLight;
layout(set = 3, binding = 1, std140) uniform NativeMaterial {
    vec4 baseColor;
    vec4 emissiveAndShading;
    vec4 surface;
} material;

float shadowVisibility(vec3 n) {
    if (shadowCoord.x <= 0.0 || shadowCoord.x >= 1.0 ||
        shadowCoord.y <= 0.0 || shadowCoord.y >= 1.0 ||
        shadowCoord.z <= 0.0 || shadowCoord.z >= 1.0) return 1.0;
    float facing = max(dot(n, normalize(sceneLight.direction.xyz)), 0.0);
    float bias = mix(0.00025, 0.00008, facing);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float visible = 0.0;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            float storedDepth = texture(
                shadowMap,
                shadowCoord.xy + (vec2(x, y) - 0.5) * texelSize).r;
            visible += shadowCoord.z - bias <= storedDepth ? 1.0 : 0.0;
        }
    }
    return visible * 0.25;
}

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
    vec3 light = mix(vec3(1.0),
        environmentLight *
            (sceneLight.ambient.rgb + sceneLight.diffuse.rgb * diffuse),
        materialLighting);
    float visibility = materialLighting > 0.001 && shadowReception > 0.5
        ? shadowVisibility(n) : 1.0;
    float shadow = mix(0.62, 1.0, visibility);
    light *= mix(shadow, 1.0, fog.a);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 modulation = min(foggedColor * 2.0, vec3(1.0));
    vec3 base = texel.rgb * modulation * light * material.baseColor.rgb;
    float roughness = clamp(material.surface.x, 0.0, 1.0);
    float metallic = clamp(material.surface.y, 0.0, 1.0);
    float gloss = 1.0 - roughness;
    float coat = smoothstep(0.18, 0.55, gloss);
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
    float reflectionStrength = coat * mix(0.10, 0.55, rim) *
        mix(0.85, 1.15, metallic);
    vec3 environmentSpecular = reflectedSky(reflect(-view, n)) *
        reflectionStrength;
    directSpecular *= coat;
    vec3 specular = (environmentSpecular + directSpecular) *
        step(0.001, materialLighting);
    vec3 emissive = texel.rgb * material.emissiveAndShading.rgb;
    outColor = vec4(base + specular + emissive,
                    texel.a * color.a * material.baseColor.a);
}
