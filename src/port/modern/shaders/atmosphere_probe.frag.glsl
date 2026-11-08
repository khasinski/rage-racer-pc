#version 450
#extension GL_GOOGLE_include_directive : require
/* The production integration function, isolated for numerical GPU readback. */
layout(set = 3, binding = 0, std140) uniform AtmosphereProbe {
    vec4 atmosphereEye;
    vec4 atmosphereRange;
    vec4 atmosphereFogColor;
    vec4 skyHorizon;
    vec4 skyTop;
    vec4 surface;
    vec4 samplePosition;
    vec4 environment;
} sceneLight;
#include "native_atmosphere.glsl"
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(applyAtmosphere(sceneLight.surface.rgb,
        sceneLight.samplePosition.xyz, sceneLight.environment.rgb,
        sceneLight.samplePosition.w), 1.0);
}
