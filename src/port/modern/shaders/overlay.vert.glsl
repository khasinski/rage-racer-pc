#version 450
// One screen-space quad of the options overlay. `rect` is x0,y0,x1,y1 in
// normalised device coordinates and `uvRect` the matching atlas region; both
// arrive per draw so the overlay needs no vertex buffer.
layout(set = 1, binding = 0, std140) uniform OverlayQuad {
    vec4 rect;
    vec4 uvRect;
    vec4 tint;
} quad;
layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 tint;
void main() {
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1),
                                    vec2(0, 1), vec2(1, 0), vec2(1, 1));
    vec2 corner = corners[gl_VertexIndex];
    gl_Position = vec4(mix(quad.rect.xy, quad.rect.zw, corner), 0.0, 1.0);
    uv = mix(quad.uvRect.xy, quad.uvRect.zw, corner);
    tint = quad.tint;
}
