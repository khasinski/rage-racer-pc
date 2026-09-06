#version 450
// Read back UV from the production shadow vertex shader without sampling.
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 unusedFog;
layout(location = 1) out vec4 unusedInstance;
layout(location = 2) out vec4 outUV;
void main() {
    unusedFog = vec4(0.0);
    unusedInstance = vec4(0.0);
    outUV = vec4(uv, 0.0, 1.0);
}
