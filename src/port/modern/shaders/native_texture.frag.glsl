#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D materialTexture;

void main() {
    vec4 texel = texture(materialTexture, uv);
    if (texel.a <= 0.001) discard;
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(vec3(-0.4, 0.7, 0.5))), 0.0);
    vec3 light = vec3(0.90 + 0.10 * diffuse);
    vec3 modulation = min(color.rgb * 2.0, vec3(1.0));
    outColor = vec4(texel.rgb * modulation * light, texel.a * color.a);
}
