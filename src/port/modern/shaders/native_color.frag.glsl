#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 fog;
layout(location = 4) in float lighting;
layout(location = 5) in vec3 environmentLight;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = dot(normal, normal) > 0.000001
        ? normalize(normal) : vec3(0.0, 1.0, 0.0);
    float diffuse = max(dot(n, normalize(vec3(-0.4, 0.7, 0.5))), 0.0);
    vec3 foggedColor = mix(color.rgb, fog.rgb, fog.a);
    vec3 light = mix(vec3(1.0),
        environmentLight * (0.35 + 0.65 * diffuse), lighting);
    outColor = vec4(foggedColor * light, color.a);
}
