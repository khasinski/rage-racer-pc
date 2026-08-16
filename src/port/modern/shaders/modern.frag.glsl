#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in uint attr;
layout(location = 3) flat in uint twin;
layout(location = 4) flat in uint clut;
layout(set = 2, binding = 0) uniform sampler2D vram;
layout(location = 0) out vec4 fragColor;

uint rgb5551(vec4 c) {
    uint r = uint(c.r * 31.0 + 0.5);
    uint g = uint(c.g * 31.0 + 0.5);
    uint b = uint(c.b * 31.0 + 0.5);
    uint a = uint(c.a + 0.5);
    return r | (g << 5u) | (b << 10u) | (a << 15u);
}

vec4 texelLookup(uvec2 pageBase, uvec2 texel, uint mode, uint palette) {
    if (mode >= 2u) return texelFetch(vram, ivec2(pageBase + texel), 0);
    uint texelShift = mode == 1u ? 1u : 2u;
    uint subMask = mode == 1u ? 1u : 3u;
    uint idxShift = mode == 1u ? 8u : 4u;
    uint idxMask = mode == 1u ? 0xffu : 0x0fu;
    uint sub = texel.x & subMask;
    uvec2 pos = uvec2(texel.x >> texelShift, texel.y);
    uint word16 = rgb5551(texelFetch(vram, ivec2(pageBase + pos), 0));
    uint colorIdx = (word16 >> (sub * idxShift)) & idxMask;
    uvec2 clutBase = uvec2((palette % 64u) * 16u, palette / 64u);
    return texelFetch(vram, ivec2(clutBase + uvec2(colorIdx, 0u)), 0);
}

void main() {
    uint tpage = attr & 0x1ffu;
    bool untextured = (attr & 0x8000u) != 0u;
    bool filterTex = (attr & 0x10000u) != 0u;
    vec4 texColor;
    if (untextured) {
        texColor = vec4(1.0, 1.0, 1.0, 2.0);
    } else {
        uvec2 twAnd = uvec2(twin & 0xffu, (twin >> 8u) & 0xffu);
        uvec2 twOr = uvec2((twin >> 16u) & 0xffu, (twin >> 24u) & 0xffu);
        uvec2 pageBase = uvec2(((tpage % 32u) % 16u) * 64u,
                               ((tpage % 32u) / 16u) * 256u);
        uint mode = (tpage >> 7u) & 3u;
        vec2 fuv = clamp(floor(uv + vec2(1.0 / 131072.0)), 0.0, 255.0);
        uvec2 texel = (uvec2(fuv) & twAnd) | twOr;
        texColor = texelLookup(pageBase, texel, mode, clut);
        if (texColor == vec4(0.0)) discard;
        if (filterTex) {
            vec2 pos = uv + vec2(1.0 / 131072.0) - 0.5;
            vec2 cell = floor(pos);
            vec2 fraction = pos - cell;
            vec3 acc = vec3(0.0);
            float weightSum = 0.0;
            for (int tap = 0; tap < 4; tap++) {
                vec2 offset = vec2(float(tap & 1), float(tap >> 1));
                vec2 at = clamp(cell + offset, 0.0, 255.0);
                vec2 axis = abs(offset - fraction);
                float weight = (1.0 - axis.x) * (1.0 - axis.y);
                uvec2 t = (uvec2(at) & twAnd) | twOr;
                vec4 c = texelLookup(pageBase, t, mode, clut);
                if (c != vec4(0.0)) {
                    acc += c.rgb * weight;
                    weightSum += weight;
                }
            }
            if (weightSum > 0.0) texColor = vec4(acc / weightSum, texColor.a);
        }
    }
    bool semiPrim = color.a < 0.75;
    bool texelSemi = texColor.a > 0.0;
    vec3 modColor;
    if (untextured) {
        modColor = color.rgb;
    } else {
        vec3 tex5 = filterTex ? texColor.rgb * 31.0
                              : floor(texColor.rgb * 31.0 + 0.5);
        vec3 col8 = min(floor(color.rgb * 255.0 + 0.5), vec3(255.0));
        vec3 prod8 = min(tex5 * col8 / 16.0, vec3(255.0));
        modColor = prod8 / 255.0;
    }
    if (texelSemi && semiPrim) {
        uint abr = (tpage & 0x60u) >> 5u;
        if (abr == 0u) fragColor = vec4(modColor * 0.5, 0.5);
        else if (abr == 3u) fragColor = vec4(modColor * 0.25, 0.0);
        else fragColor = vec4(modColor, 0.0);
    } else {
        fragColor = vec4(modColor, 1.0);
    }
}
