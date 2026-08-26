#version 450

layout(location = 0) in vec2 uv;
layout(set = 2, binding = 0) uniform sampler2D textureImage;

void main() {
    if (texture(textureImage, uv).a <= 0.5) {
        discard;
    }
}
