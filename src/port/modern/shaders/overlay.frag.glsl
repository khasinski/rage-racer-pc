#version 450
layout(set = 2, binding = 0) uniform sampler2D atlas;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 tint;
layout(location = 0) out vec4 fragColor;
// The atlas holds premultiplied RGBA; the tint scales colour and coverage
// together so a fade needs no second blend mode.
void main() {
    fragColor = texture(atlas, uv) * tint;
}
