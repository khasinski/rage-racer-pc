// Shared by the ambient-occlusion passes. Depth arrives as the renderer's
// ordinary perspective depth: ndc = projection.z + projection.w / viewDepth.
layout(set = 3, binding = 0, std140) uniform Ssao {
    vec4 projection;  // xy: view -> clip scale, zw: depth terms
    vec4 sampling;    // x: world radius, y: strength, z: fade start, w: fade end
    vec4 screen;      // xy: texel size, zw: blur step (texels) for this pass
} ssao;

const float SKY_DEPTH = 0.99999;

float linearDepth(float ndcDepth) {
    return ssao.projection.w / (ndcDepth - ssao.projection.z);
}

// Position in view space (camera at the origin, looking down -Z) of the
// texel at `st`, given its linear depth.
vec3 viewPosition(vec2 st, float depth) {
    vec2 ndc = vec2(st.x * 2.0 - 1.0, 1.0 - st.y * 2.0);
    return vec3(ndc * depth / ssao.projection.xy, -depth);
}
