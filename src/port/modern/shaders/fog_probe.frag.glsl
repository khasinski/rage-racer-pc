#version 450
// Test readback of the production native vertex shader's fog output.
layout(location = 3) in vec4 fog;
layout(location = 0) out vec4 outColor;
void main() { outColor = fog; }
