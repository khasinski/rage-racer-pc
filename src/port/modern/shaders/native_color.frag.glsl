#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 7) in float shadowReception;
layout(location = 9) in vec3 worldPositionIn;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2DShadow shadowMap;
#include "native_scene_light.glsl"
#include "native_spot.glsl"
#include "native_sun_shadow.glsl"

void main() {
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(sceneLight.direction.xyz)), 0.0);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    float visibility = 1.0;
    if (shadowReception > 0.5 && lighting > 0.0 && fog.a < 1.0) {
        visibility = sunVisibility(worldPositionIn, n);
    }
    float shadow = mix(0.10, 1.0, visibility);
    float ambientShadow = mix(0.55, 1.0, visibility);
    vec3 light = mix(vec3(1.0),
        environmentLight * (sceneLight.ambient.rgb * ambientShadow +
            sceneLight.diffuse.rgb * diffuse * shadow),
        lighting);
    /* See native_texture.frag.glsl: baked vertex colours need the extra term. */
    light *= mix(1.0, mix(0.66, 1.0, visibility), lighting);
    // Preserve texture contrast, but keep indirect light readable in tunnels.
    light = max(light, vec3(0.18));
    light = mix(light, vec3(1.0), fog.a);
    outColor = vec4(foggedColor * light + color.rgb *
        spotLighting(worldPositionIn, n) * (1.0 - fog.a), color.a);
}
