#include "modern_shader_sources.h"


/* Post-processing: a fullscreen pass over the finished frame. One built-in
 * effect for now (FXAA edge anti-aliasing); the pass is the extension point
 * for further effects. */

/* Fullscreen vertex used by the optional colour grading pass. */
const char MODERN_EFFECTS_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct PostOut { float4 pos [[position]]; float2 uv; };\n"
    "vertex PostOut vs_fx(uint vid [[vertex_id]]) {\n"
    "    PostOut out;\n"
    "    float2 corner = float2((vid << 1) & 2, vid & 2);\n"
    "    out.pos = float4(corner * 2.0 - 1.0, 0.0, 1.0);\n"
    "    out.uv = float2(corner.x, 1.0 - corner.y);\n"
    "    return out;\n"
    "}\n";

/* Composite prologue + body; kGrading is inserted between them. */
const char MODERN_COMPOSITE_PROLOGUE_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n";
const char MODERN_COMPOSITE_MSL[] =
    "struct PostOut { float4 pos [[position]]; float2 uv; };\n"
    "fragment float4 fs_composite(PostOut in [[stage_in]],\n"
    "                             texture2d<float> frame [[texture(0)]],\n"
    "                             sampler smpFrame [[sampler(0)]]) {\n"
    "    float3 c = frame.sample(smpFrame, in.uv).rgb;\n"
    "    if (kGrading != 0) {\n"
    "        float luma = dot(c, float3(0.299, 0.587, 0.114));\n"
    "        c = mix(float3(luma), c, 1.16);\n"
    "        c = (c - 0.5) * 1.04 + 0.5;\n"
    "    }\n"
    "    return float4(saturate(c), 1.0);\n"
    "}\n";

const size_t MODERN_EFFECTS_MSL_SIZE = sizeof(MODERN_EFFECTS_MSL);
const size_t MODERN_COMPOSITE_PROLOGUE_MSL_SIZE = sizeof(MODERN_COMPOSITE_PROLOGUE_MSL);
const size_t MODERN_COMPOSITE_MSL_SIZE = sizeof(MODERN_COMPOSITE_MSL);
