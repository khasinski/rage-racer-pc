#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in uvec4 inColor;
layout(location = 3) in uint inAttr;
layout(location = 4) in uint inTwin;
layout(location = 5) in uint inClut;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;
layout(location = 2) flat out uint attr;
layout(location = 3) flat out uint twin;
layout(location = 4) flat out uint clut;

void main() {
    gl_Position = inPos;
    color = vec4(inColor) / 255.0;
    uv = inUV;
    attr = inAttr;
    twin = inTwin;
    clut = inClut;
}
