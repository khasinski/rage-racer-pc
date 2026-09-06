#version 450
// Test readback of the production native vertex shader's fog output.
layout(location = 3) in vec4 fog;
layout(location = 0) out vec4 outColor;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 7) in float shadowReception;
layout(location = 1) out vec4 outInstance;
void main() {
    outColor = fog;
    outInstance = vec4(environmentLight * lighting, shadowReception);
}
