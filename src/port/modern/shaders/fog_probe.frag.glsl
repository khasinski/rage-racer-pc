#version 450
// Test readback of the production native vertex shader's fog output.
layout(location = 3) in vec4 fog;
layout(location = 0) out vec4 outColor;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 7) in float shadowReception;
layout(location = 1) out vec4 outInstance;
layout(location = 0) in vec2 uv;
layout(location = 2) out vec4 outUV;
void main() {
    outColor = fog;
    outInstance = vec4(environmentLight * lighting, shadowReception);
    outUV = vec4(uv, 0.0, 1.0);
    if (lighting < 0.0) {
        uint bits = floatBitsToUint(gl_FragCoord.z);
        outColor = vec4(uvec4(bits, bits >> 8, bits >> 16, bits >> 24) & 255u) / 255.0;
    }
}
