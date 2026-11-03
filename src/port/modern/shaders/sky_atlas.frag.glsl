#version 450
// Use captured sky polygons and native tile UVs, including camera roll/pitch.
layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in uint attr;
layout(location = 0) out vec4 fragColor;
layout(set = 2, binding = 0) uniform sampler2D atlas;
void main() {
    // Upload is premultiplied, so filtered transparent texels cannot add halos.
    vec4 texel = texture(atlas, uv / 256.0);
    if(texel.a <= 0.001) discard;
    vec3 rgb = min(texel.rgb * color.rgb * (31.0/16.0), vec3(texel.a));
    if(color.a < 0.75) {
        uint abr = (attr >> 5u) & 3u;
        if(abr == 0u) fragColor = vec4(rgb * 0.5, texel.a * 0.5);
        else fragColor = vec4(rgb * (abr == 3u ? 0.25 : 1.0), 0.0);
    } else fragColor = vec4(rgb, texel.a);
}
