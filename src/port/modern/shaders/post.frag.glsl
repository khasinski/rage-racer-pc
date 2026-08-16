#version 450
layout(location = 0) in vec2 uv;
layout(set = 2, binding = 0) uniform sampler2D frame;
layout(location = 0) out vec4 fragColor;
float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
void main() {
    vec2 texel = 1.0 / vec2(textureSize(frame, 0));
    vec3 nw = texture(frame, uv + vec2(-1, -1) * texel).rgb;
    vec3 ne = texture(frame, uv + vec2( 1, -1) * texel).rgb;
    vec3 sw = texture(frame, uv + vec2(-1,  1) * texel).rgb;
    vec3 se = texture(frame, uv + vec2( 1,  1) * texel).rgb;
    vec4 center = texture(frame, uv);
    float lnw = luma(nw), lne = luma(ne), lsw = luma(sw), lse = luma(se);
    float lm = luma(center.rgb);
    float lo = min(lm, min(min(lnw, lne), min(lsw, lse)));
    float hi = max(lm, max(max(lnw, lne), max(lsw, lse)));
    vec2 dir = vec2(-((lnw + lne) - (lsw + lse)),
                     (lnw + lsw) - (lne + lse));
    float reduce = max((lnw + lne + lsw + lse) * 0.25 / 8.0, 1.0 / 128.0);
    dir = clamp(dir / (min(abs(dir.x), abs(dir.y)) + reduce),
                vec2(-8.0), vec2(8.0)) * texel;
    vec3 a = 0.5 * (texture(frame, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
                    texture(frame, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 b = a * 0.5 + 0.25 * (texture(frame, uv - dir * 0.5).rgb +
                               texture(frame, uv + dir * 0.5).rgb);
    float lb = luma(b);
    fragColor = vec4((lb < lo || lb > hi) ? a : b, center.a);
}
