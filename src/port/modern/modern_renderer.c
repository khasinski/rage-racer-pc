#include "modern_renderer.h"

#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>
#include <psyz/video.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scene_capture.h"

/* The modern renderer (plan phase R2). Consumes the RageScene snapshot the
 * capture layer records each logic frame and renders it with float
 * transforms, a real depth buffer and perspective-correct texturing into an
 * offscreen target, which the PsyZ present-source hook then presents in
 * place of the PS1 VRAM image. Scenes that submit no 3D pass through to the
 * compat image untouched (menus, FMV, 480-line screens).
 *
 * Textures are sampled live from PsyZ's emulated VRAM with CLUT
 * indirection, exactly like the compat rasterizer, so no texture cache or
 * invalidation is needed; VRAM content lags one presented frame, which only
 * affects textures drawn by GP0 rendering, not asset uploads. */

static int s_enabled;
static SDL_Window *s_window;
static SDL_GPUDevice *s_device;
static PsyzOverlayInitCB_SDL3GPU s_prev_overlay_init;
static PsyzPresentSourceCB_SDL3GPU s_prev_present_source;
static RagePortConfig s_config;

/* ---- GPU resources ---- */

static int s_targetW, s_targetH;
static float s_logicalW = 320.0f; /* 16:9 widens this; 240 rows stay */
static float s_overscanX;         /* (logicalW - 320) / 2 */
static SDL_GPUTexture *s_target;
static SDL_GPUTexture *s_depth;
static SDL_GPUSampler *s_sampler;
static SDL_GPUGraphicsPipeline *s_pipe3dOpaque;
static SDL_GPUGraphicsPipeline *s_pipe3dBlend;
static SDL_GPUGraphicsPipeline *s_pipe3dSub;
static SDL_GPUGraphicsPipeline *s_pipe2d;
static SDL_GPUGraphicsPipeline *s_pipe2dSub;
static SDL_GPUBuffer *s_vertexBuffer;
static SDL_GPUTransferBuffer *s_vertexTransfer;
static SDL_GPUTexture *s_postTarget;
static SDL_GPUGraphicsPipeline *s_pipePost;
static SDL_GPUSampler *s_samplerLinear;
static int s_resourcesReady;
static uint32_t s_lastRenderedFrame = 0xFFFFFFFFu;
static int s_haveRenderedFrame;

typedef struct ModernVertex {
    float x, y, z, w; /* clip space */
    float u, v;       /* texture-page texel coordinates */
    uint8_t color[4]; /* rgb + semi flag in alpha (0 semi, 255 opaque) */
    uint32_t attr;    /* tpage | 0x8000 when untextured */
    uint32_t twin;    /* texture window {andX, andY, orX, orY} bytes */
    uint32_t clut;
} ModernVertex;

enum {
    MODERN_PIPE_3D_OPAQUE,
    MODERN_PIPE_3D_BLEND,
    MODERN_PIPE_3D_SUB,
    MODERN_PIPE_2D,
    MODERN_PIPE_2D_SUB,
};

typedef struct ModernSpan {
    uint8_t pipeline;
    uint8_t hasScissor;
    uint8_t pass; /* 0 = main scene, 1 = mirror overlay (depth recleared) */
    SDL_Rect scissor;
    int32_t start, count;
    float depthKey; /* semi-transparent 3D sorting */
} ModernSpan;

#define MODERN_MAX_VERTICES 400000
#define MODERN_MAX_SPANS 16384

static ModernVertex *s_vertices;
static int s_vertexCount;
static ModernSpan *s_spans;
static int s_spanCount;

/* The far bucket boundary: captured 2D packets at or beyond this ordering
 * table index draw behind the 3D scene (sky layers); everything nearer
 * draws over it. */
#define MODERN_BACKGROUND_BUCKET 576

#define MODERN_NEAR 16.0f
#define MODERN_FAR 262144.0f

/* Depth-key range: ordering keys are compat bucket windows in z units and
 * may be negative (the +128 ordering-table base admits ot[-128..-1], which
 * the player car's biased faces use). Mapped linearly into [0,1]. */
#define MODERN_DEPTH_MIN (-80000.0f)
#define MODERN_DEPTH_RANGE (200000.0f)

/* ---- MSL shaders ---- */

static const char MODERN_SHADER_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct VSIn {\n"
    "    float4 pos [[attribute(0)]];\n"
    "    float2 uv [[attribute(1)]];\n"
    "    uchar4 color [[attribute(2)]];\n"
    "    uint attr [[attribute(3)]];\n"
    "    uint twin [[attribute(4)]];\n"
    "    uint clut [[attribute(5)]];\n"
    "};\n"
    "struct VSOut {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "    float2 uv;\n"
    "    uint attr [[flat]];\n"
    "    uint twin [[flat]];\n"
    "    uint clut [[flat]];\n"
    "};\n"
    "vertex VSOut vs_main(VSIn in [[stage_in]]) {\n"
    "    VSOut out;\n"
    "    out.pos = in.pos;\n"
    "    out.color = float4(in.color) / 255.0;\n"
    "    out.uv = in.uv;\n"
    "    out.attr = in.attr;\n"
    "    out.twin = in.twin;\n"
    "    out.clut = in.clut;\n"
    "    return out;\n"
    "}\n"
    "static uint rgb5551(float4 c) {\n"
    "    uint r = uint(c.r * 31.0 + 0.5);\n"
    "    uint g = uint(c.g * 31.0 + 0.5);\n"
    "    uint b = uint(c.b * 31.0 + 0.5);\n"
    "    uint a = uint(c.a + 0.5);\n"
    "    return r | (g << 5) | (b << 10) | (a << 15);\n"
    "}\n"
    "fragment float4 fs_main(VSOut in [[stage_in]],\n"
    "                        texture2d<float> vram [[texture(0)]],\n"
    "                        sampler smp [[sampler(0)]]) {\n"
    "    uint texWord = in.attr;\n"
    "    uint tpage = texWord & 0x1FFu;\n"
    "    bool untextured = (texWord & 0x8000u) != 0u;\n"
    "    float4 texColor;\n"
    "    if (untextured) {\n"
    "        texColor = float4(1.0, 1.0, 1.0, 2.0);\n"
    "    } else {\n"
    "        uint2 twAnd = uint2(in.twin & 0xFFu, (in.twin >> 8) & 0xFFu);\n"
    "        uint2 twOr = uint2((in.twin >> 16) & 0xFFu,\n"
    "                           (in.twin >> 24) & 0xFFu);\n"
    "        uint2 pageBase = uint2(((tpage % 32u) % 16u) * 64u,\n"
    "                               ((tpage % 32u) / 16u) * 256u);\n"
    "        float2 fuv = clamp(floor(in.uv + float2(1.0 / 131072.0)),\n"
    "                           0.0, 255.0);\n"
    "        uint2 texel = (uint2(fuv) & twAnd) | twOr;\n"
    "        uint mode = (tpage >> 7) & 3u;\n"
    "        if (mode >= 2u) {\n"
    "            texColor = vram.read(pageBase + texel);\n"
    "        } else {\n"
    "            uint texelShift = (mode == 1u) ? 1u : 2u;\n"
    "            uint subMask = (mode == 1u) ? 1u : 3u;\n"
    "            uint idxShift = (mode == 1u) ? 8u : 4u;\n"
    "            uint idxMask = (mode == 1u) ? 0xFFu : 0xFu;\n"
    "            uint sub = texel.x & subMask;\n"
    "            uint2 pos = uint2(texel.x >> texelShift, texel.y);\n"
    "            uint word16 = rgb5551(vram.read(pageBase + pos));\n"
    "            uint colorIdx = (word16 >> (sub * idxShift)) & idxMask;\n"
    "            uint2 clutBase = uint2((in.clut % 64u) * 16u,\n"
    "                                   in.clut / 64u);\n"
    "            texColor = vram.read(clutBase + uint2(colorIdx, 0u));\n"
    "        }\n"
    "        if (all(texColor == float4(0.0))) {\n"
    "            discard_fragment();\n"
    "        }\n"
    "    }\n"
    "    bool semiPrim = in.color.a < 0.75;\n"
    "    bool texelSemi = texColor.a > 0.0;\n"
    "    float3 modColor;\n"
    "    if (untextured) {\n"
    "        modColor = in.color.rgb;\n"
    "    } else {\n"
    "        float3 tex5 = floor(texColor.rgb * 31.0 + 0.5);\n"
    "        float3 col8 = min(floor(in.color.rgb * 255.0 + 0.5),\n"
    "                          float3(255.0));\n"
    "        float3 prod8 = min(tex5 * col8 / 16.0, float3(255.0));\n"
    "        modColor = prod8 / 255.0;\n"
    "    }\n"
    "    if (texelSemi && semiPrim) {\n"
    "        uint abr = (tpage & 0x60u) >> 5u;\n"
    "        if (abr == 0u) {\n"
    "            return float4(modColor * 0.5, 0.5);\n"
    "        } else if (abr == 3u) {\n"
    "            return float4(modColor * 0.25, 0.0);\n"
    "        }\n"
    "        return float4(modColor, 0.0);\n"
    "    }\n"
    "    return float4(modColor, 1.0);\n"
    "}\n";

/* Post-processing: a fullscreen pass over the finished frame. One built-in
 * effect for now (FXAA edge anti-aliasing); the pass is the extension point
 * for further effects. */
static const char MODERN_POST_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct PostOut { float4 pos [[position]]; float2 uv; };\n"
    "vertex PostOut vs_post(uint vid [[vertex_id]]) {\n"
    "    PostOut out;\n"
    "    float2 corner = float2((vid << 1) & 2, vid & 2);\n"
    "    out.pos = float4(corner * 2.0 - 1.0, 0.0, 1.0);\n"
    "    out.uv = float2(corner.x, 1.0 - corner.y);\n"
    "    return out;\n"
    "}\n"
    "static float fxaaLuma(float3 c) {\n"
    "    return dot(c, float3(0.299, 0.587, 0.114));\n"
    "}\n"
    "fragment float4 fs_post(PostOut in [[stage_in]],\n"
    "                        texture2d<float> frame [[texture(0)]],\n"
    "                        sampler smp [[sampler(0)]]) {\n"
    "    float2 texel = 1.0 / float2(frame.get_width(), frame.get_height());\n"
    "    float3 rgbNW = frame.sample(smp, in.uv + float2(-1, -1) * texel).rgb;\n"
    "    float3 rgbNE = frame.sample(smp, in.uv + float2(1, -1) * texel).rgb;\n"
    "    float3 rgbSW = frame.sample(smp, in.uv + float2(-1, 1) * texel).rgb;\n"
    "    float3 rgbSE = frame.sample(smp, in.uv + float2(1, 1) * texel).rgb;\n"
    "    float4 center = frame.sample(smp, in.uv);\n"
    "    float lumaNW = fxaaLuma(rgbNW), lumaNE = fxaaLuma(rgbNE);\n"
    "    float lumaSW = fxaaLuma(rgbSW), lumaSE = fxaaLuma(rgbSE);\n"
    "    float lumaM = fxaaLuma(center.rgb);\n"
    "    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE),\n"
    "                                   min(lumaSW, lumaSE)));\n"
    "    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE),\n"
    "                                   max(lumaSW, lumaSE)));\n"
    "    float2 dir = float2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),\n"
    "                        (lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
    "    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *\n"
    "                              0.25 * (1.0 / 8.0),\n"
    "                          1.0 / 128.0);\n"
    "    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
    "    dir = clamp(dir * rcpDirMin, float2(-8.0), float2(8.0)) * texel;\n"
    "    float3 rgbA = 0.5 *\n"
    "        (frame.sample(smp, in.uv + dir * (1.0 / 3.0 - 0.5)).rgb +\n"
    "         frame.sample(smp, in.uv + dir * (2.0 / 3.0 - 0.5)).rgb);\n"
    "    float3 rgbB = rgbA * 0.5 + 0.25 *\n"
    "        (frame.sample(smp, in.uv + dir * -0.5).rgb +\n"
    "         frame.sample(smp, in.uv + dir * 0.5).rgb);\n"
    "    float lumaB = fxaaLuma(rgbB);\n"
    "    float3 result = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;\n"
    "    return float4(result, center.a);\n"
    "}\n";

/* ---- resource creation ---- */

static SDL_GPUShader *ModernCreateShader(const char *source, size_t sourceSize,
                                         SDL_GPUShaderStage stage,
                                         const char *entry, int samplers) {
    SDL_GPUShaderCreateInfo info = {0};
    info.code = (const Uint8 *)source;
    info.code_size = sourceSize;
    info.entrypoint = entry;
    info.format = SDL_GPU_SHADERFORMAT_MSL;
    info.stage = stage;
    info.num_samplers = (Uint32)samplers;
    return SDL_CreateGPUShader(s_device, &info);
}

static SDL_GPUGraphicsPipeline *ModernCreatePostPipeline(void) {
    SDL_GPUShader *vs =
        ModernCreateShader(MODERN_POST_MSL, sizeof(MODERN_POST_MSL),
                           SDL_GPU_SHADERSTAGE_VERTEX, "vs_post", 0);
    SDL_GPUShader *fs =
        ModernCreateShader(MODERN_POST_MSL, sizeof(MODERN_POST_MSL),
                           SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_post", 1);
    SDL_GPUGraphicsPipeline *pipeline = NULL;
    if (vs && fs) {
        const SDL_GPUColorTargetDescription target = {
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        };
        SDL_GPUGraphicsPipelineCreateInfo info = {0};
        info.vertex_shader = vs;
        info.fragment_shader = fs;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(s_device, &info);
    }
    if (vs) SDL_ReleaseGPUShader(s_device, vs);
    if (fs) SDL_ReleaseGPUShader(s_device, fs);
    return pipeline;
}

static SDL_GPUGraphicsPipeline *ModernCreatePipeline(
    SDL_GPUShader *vs, SDL_GPUShader *fs, int depthTest, int depthWrite,
    int subtract) {
    const SDL_GPUVertexBufferDescription buffer = {
        .slot = 0,
        .pitch = sizeof(ModernVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    const SDL_GPUVertexAttribute attributes[] = {
        {.location = 0, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
         .offset = offsetof(ModernVertex, x)},
        {.location = 1, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
         .offset = offsetof(ModernVertex, u)},
        {.location = 2, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4,
         .offset = offsetof(ModernVertex, color)},
        {.location = 3, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
         .offset = offsetof(ModernVertex, attr)},
        {.location = 4, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
         .offset = offsetof(ModernVertex, twin)},
        {.location = 5, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
         .offset = offsetof(ModernVertex, clut)},
    };
    const SDL_GPUBlendOp op = subtract ? SDL_GPU_BLENDOP_REVERSE_SUBTRACT
                                       : SDL_GPU_BLENDOP_ADD;
    const SDL_GPUColorTargetDescription target = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = op,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = op,
            .enable_blend = true,
        },
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {0};
    info.vertex_shader = vs;
    info.fragment_shader = fs;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_attributes =
        sizeof(attributes) / sizeof(attributes[0]);
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.depth_stencil_state.compare_op =
        depthTest ? SDL_GPU_COMPAREOP_LESS_OR_EQUAL
                  : SDL_GPU_COMPAREOP_ALWAYS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = depthWrite != 0;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(s_device, &info);
}

static int ModernEnsureResources(void) {
    SDL_GPUShader *vs;
    SDL_GPUShader *fs;
    float scale;
    if (s_resourcesReady) return 1;
    if (s_device == NULL) return 0;

    scale = s_config.modernInternalScale;
    if (scale < 0.5f) scale = 0.5f;
    /* 16:9 widens the field of view; the vertical view is unchanged. */
    s_logicalW = s_config.modernAspect == RAGE_MODERN_ASPECT_16_9
                     ? 320.0f * (16.0f / 9.0f) / (4.0f / 3.0f)
                     : 320.0f;
    s_overscanX = (s_logicalW - 320.0f) * 0.5f;
    s_targetW = (int)(s_logicalW * scale + 0.5f) & ~1;
    s_targetH = (int)(240.0f * scale + 0.5f) & ~1;

    {
        SDL_GPUTextureCreateInfo info = {0};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_targetW;
        info.height = (Uint32)s_targetH;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_target = SDL_CreateGPUTexture(s_device, &info);
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        s_depth = SDL_CreateGPUTexture(s_device, &info);
    }
    {
        SDL_GPUSamplerCreateInfo info = {0};
        info.min_filter = SDL_GPU_FILTER_NEAREST;
        info.mag_filter = SDL_GPU_FILTER_NEAREST;
        s_sampler = SDL_CreateGPUSampler(s_device, &info);
    }
    vs = ModernCreateShader(MODERN_SHADER_MSL, sizeof(MODERN_SHADER_MSL),
                            SDL_GPU_SHADERSTAGE_VERTEX, "vs_main", 0);
    fs = ModernCreateShader(MODERN_SHADER_MSL, sizeof(MODERN_SHADER_MSL),
                            SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_main", 1);
    if (vs && fs) {
        s_pipe3dOpaque = ModernCreatePipeline(vs, fs, 1, 1, 0);
        s_pipe3dBlend = ModernCreatePipeline(vs, fs, 1, 0, 0);
        s_pipe3dSub = ModernCreatePipeline(vs, fs, 1, 0, 1);
        s_pipe2d = ModernCreatePipeline(vs, fs, 0, 0, 0);
        s_pipe2dSub = ModernCreatePipeline(vs, fs, 0, 0, 1);
    }
    if (vs) SDL_ReleaseGPUShader(s_device, vs);
    if (fs) SDL_ReleaseGPUShader(s_device, fs);
    {
        SDL_GPUBufferCreateInfo info = {0};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = MODERN_MAX_VERTICES * sizeof(ModernVertex);
        s_vertexBuffer = SDL_CreateGPUBuffer(s_device, &info);
    }
    {
        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = MODERN_MAX_VERTICES * sizeof(ModernVertex);
        s_vertexTransfer = SDL_CreateGPUTransferBuffer(s_device, &info);
    }
    s_vertices = malloc(MODERN_MAX_VERTICES * sizeof(ModernVertex));
    s_spans = malloc(MODERN_MAX_SPANS * sizeof(ModernSpan));

    if (s_config.modernPost != RAGE_MODERN_POST_NONE) {
        SDL_GPUTextureCreateInfo info = {0};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_targetW;
        info.height = (Uint32)s_targetH;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_postTarget = SDL_CreateGPUTexture(s_device, &info);
        s_pipePost = ModernCreatePostPipeline();
        {
            SDL_GPUSamplerCreateInfo samplerInfo = {0};
            samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
            samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
            s_samplerLinear = SDL_CreateGPUSampler(s_device, &samplerInfo);
        }
        if (!s_postTarget || !s_pipePost || !s_samplerLinear) {
            fprintf(stderr,
                    "rage-port: post-process setup failed, disabling: %s\n",
                    SDL_GetError());
            s_config.modernPost = RAGE_MODERN_POST_NONE;
        }
    }

    if (!s_target || !s_depth || !s_sampler || !s_pipe3dOpaque ||
        !s_pipe3dBlend || !s_pipe3dSub || !s_pipe2d || !s_pipe2dSub ||
        !s_vertexBuffer || !s_vertexTransfer || !s_vertices || !s_spans) {
        fprintf(stderr, "rage-port: modern renderer resource setup failed: %s\n",
                SDL_GetError());
        return 0;
    }
    s_resourcesReady = 1;
    fprintf(stderr, "rage-port: modern renderer target %dx%d\n", s_targetW,
            s_targetH);
    return 1;
}

/* ---- frame building ---- */

typedef struct Modern2DState {
    uint32_t tpage;    /* from GP0(E1) for sprites */
    uint32_t twin;     /* current texture window bytes */
    SDL_Rect scissor;  /* current draw area, overlay pixels */
    int hasScissor;
    int areaEmpty;
    int offsetX, offsetY; /* GP0(E5) relative offset */
} Modern2DState;

/* GPU state at the end of the main ordering table, inherited by the mirror
 * table exactly like the real GPU carries it across DrawOTag calls. */
static Modern2DState s_mainEnd2D;

static uint8_t s_currentPass;

static ModernSpan *ModernBeginSpan(int pipeline, const Modern2DState *state,
                                   float depthKey) {
    ModernSpan *span;
    if (s_spanCount > 0) {
        span = &s_spans[s_spanCount - 1];
        if (span->pipeline == pipeline && depthKey == span->depthKey &&
            span->pass == s_currentPass &&
            ((state == NULL && !span->hasScissor) ||
             (state != NULL && state->hasScissor == span->hasScissor &&
              (!state->hasScissor ||
               (state->scissor.x == span->scissor.x &&
                state->scissor.y == span->scissor.y &&
                state->scissor.w == span->scissor.w &&
                state->scissor.h == span->scissor.h))))) {
            return span;
        }
    }
    if (s_spanCount >= MODERN_MAX_SPANS) return NULL;
    span = &s_spans[s_spanCount++];
    span->pipeline = (uint8_t)pipeline;
    span->pass = s_currentPass;
    span->hasScissor = state != NULL && state->hasScissor;
    if (span->hasScissor) span->scissor = state->scissor;
    span->start = s_vertexCount;
    span->count = 0;
    span->depthKey = depthKey;
    return span;
}

static int ModernPushVertices(ModernSpan *span, const ModernVertex *v,
                              int count) {
    if (span == NULL) return 0;
    if (s_vertexCount + count > MODERN_MAX_VERTICES) return 0;
    memcpy(&s_vertices[s_vertexCount], v, count * sizeof(*v));
    s_vertexCount += count;
    span->count += count;
    return 1;
}

/* Quad corners arrive in PS1 order (0,1,2,3 = top-left, top-right,
 * bottom-left, bottom-right); triangles are {0,1,2} and {1,3,2}. */
static void ModernEmitQuad(ModernSpan *span, const ModernVertex corners[4]) {
    ModernVertex tri[6];
    tri[0] = corners[0];
    tri[1] = corners[1];
    tri[2] = corners[2];
    tri[3] = corners[1];
    tri[4] = corners[3];
    tri[5] = corners[2];
    ModernPushVertices(span, tri, 6);
}

static void ModernEmitTriangle(ModernSpan *span,
                               const ModernVertex corners[3]) {
    ModernPushVertices(span, corners, 3);
}

static uint32_t ModernTwinFromE2(uint32_t word) {
    uint32_t maskX = word & 0x1Fu;
    uint32_t maskY = (word >> 5) & 0x1Fu;
    uint32_t offX = (word >> 10) & 0x1Fu;
    uint32_t offY = (word >> 15) & 0x1Fu;
    if (maskX == 0 && maskY == 0) return 0x0000FFFFu;
    return ((~(maskX * 8) & 0xFFu)) | ((~(maskY * 8) & 0xFFu) << 8) |
           (((offX & maskX) * 8) << 16) | (((offY & maskY) * 8) << 24);
}

/* ---- snapshot interpolation (arbitrary-FPS presentation) ---- */

static int s_useLerp;
static RageCaptureGteState s_lerpDrawGte[RAGE_CAPTURE_MAX_DRAWS];
static RageCaptureTerrainBatch s_lerpTerrain[RAGE_CAPTURE_MAX_TERRAIN];
static Uint64 s_tickTimeNs;
static Uint64 s_tickIntervalNs;
static uint32_t s_tickFrame = 0xFFFFFFFFu;

static void ModernLerpGte(const RageCaptureGteState *a,
                          const RageCaptureGteState *b, float t,
                          RageCaptureGteState *out) {
    int row, column, axis;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            out->rot.m[row][column] =
                (int16_t)(a->rot.m[row][column] +
                          (b->rot.m[row][column] - a->rot.m[row][column]) * t);
            out->light.m[row][column] = b->light.m[row][column];
            out->color.m[row][column] = b->color.m[row][column];
        }
    }
    for (axis = 0; axis < 3; axis++) {
        out->rot.t[axis] =
            (int32_t)(a->rot.t[axis] +
                      (b->rot.t[axis] - a->rot.t[axis]) * (double)t);
        out->light.t[axis] = b->light.t[axis];
        out->color.t[axis] = b->color.t[axis];
    }
    out->ofx = b->ofx;
    out->ofy = b->ofy;
    out->h = b->h;
    out->dqa = b->dqa;
    out->dqb = b->dqb;
}

/* Lerp the BASE frame's draws/terrain toward the newer TARGET frame. The
 * rendered face set is the base's: its trailing edge (nearest road, faces
 * the target frame already culled) is clipped naturally by the GPU instead
 * of vanishing for a whole logic tick, and the leading edge only pops far
 * away at the horizon. Camera cuts and scene changes snap instead. */
static void ModernPrepareInterpolation(const RageSceneSnapshot *base,
                                       const RageSceneSnapshot *target,
                                       float t) {
    int i;
    int timerDelta = target->sceneTimer - base->sceneTimer;
    long long cameraDelta = 0;
    s_useLerp = 0;
    if (t <= 0.001f) return;
    if (target->sceneId != base->sceneId) return;
    if (timerDelta < 0 || timerDelta > 2) return;
    for (i = 0; i < 3; i++) {
        long long delta = (long long)target->viewPosition[i] -
                          base->viewPosition[i];
        cameraDelta += delta < 0 ? -delta : delta;
    }
    if (cameraDelta > 16384) return;
    for (i = 0; i < base->drawCount; i++) {
        const RageCaptureModelDraw *draw = &base->draws[i];
        const RageCaptureModelDraw *match = NULL;
        long long bestDistance = 0;
        int j;
        /* Match by nearest translation among same-key candidates. The four
         * wheels of one car share a model index, and occurrence-order
         * pairing slipped by one whenever a wheel was culled for a frame,
         * lerping each wheel toward another corner (wheels flashing through
         * the body, mirrored rivals spinning the wrong way). */
        for (j = 0; j < target->drawCount; j++) {
            const RageCaptureModelDraw *other = &target->draws[j];
            long long distance = 0;
            int axis;
            if (other->kind != draw->kind ||
                other->modelIndex != draw->modelIndex ||
                other->mirror != draw->mirror ||
                other->table != draw->table) {
                continue;
            }
            for (axis = 0; axis < 3; axis++) {
                long long delta = (long long)other->gte.rot.t[axis] -
                                  draw->gte.rot.t[axis];
                distance += delta * delta;
            }
            if (match == NULL || distance < bestDistance) {
                match = other;
                bestDistance = distance;
            }
        }
        /* Snap draws whose orientation changed too much in one tick:
         * lerping across a large rotation produces inverted-looking spin. */
        if (match != NULL) {
            long dot = 0;
            int axis;
            for (axis = 0; axis < 3; axis++) {
                dot += (long)draw->gte.rot.m[0][axis] *
                       match->gte.rot.m[0][axis];
                dot += (long)draw->gte.rot.m[2][axis] *
                       match->gte.rot.m[2][axis];
            }
            if (dot < 0) match = NULL;
        }
        if (match != NULL) {
            ModernLerpGte(&draw->gte, &match->gte, t, &s_lerpDrawGte[i]);
        } else {
            s_lerpDrawGte[i] = draw->gte;
        }
    }
    for (i = 0; i < base->terrainCount; i++) {
        const RageCaptureTerrainBatch *batch = &base->terrain[i];
        const RageCaptureTerrainBatch *match =
            i < target->terrainCount &&
                    target->terrain[i].mirror == batch->mirror
                ? &target->terrain[i]
                : NULL;
        int cell;
        s_lerpTerrain[i] = *batch;
        if (match == NULL) continue;
        ModernLerpGte(&batch->gte, &match->gte, t, &s_lerpTerrain[i].gte);
        for (cell = 0; cell < batch->cellCount; cell++) {
            int other;
            for (other = 0; other < match->cellCount; other++) {
                if (match->cells[other][3] == batch->cells[cell][3]) {
                    int axis;
                    for (axis = 0; axis < 3; axis++) {
                        s_lerpTerrain[i].cells[cell][axis] = (int32_t)(
                            batch->cells[cell][axis] +
                            (match->cells[other][axis] -
                             batch->cells[cell][axis]) *
                                (double)t);
                    }
                    break;
                }
            }
        }
    }
    s_useLerp = 1;
}

/* ---- 3D faces ---- */

typedef struct ModernFaceOrder {
    int32_t index;
    float depth;
} ModernFaceOrder;

static int ModernCompareFaceOrder(const void *a, const void *b) {
    const ModernFaceOrder *fa = a, *fb = b;
    if (fa->depth > fb->depth) return -1;
    if (fa->depth < fb->depth) return 1;
    return fa->index < fb->index ? -1 : 1;
}

static void ModernFaceViewPositions(const RageSceneSnapshot *snapshot,
                                    const RageCaptureFace *face,
                                    float out[4][3], int *mirror) {
    const RageCaptureGteState *gte;
    float tx, ty, tz;
    int vertex;
    if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        const RageCaptureTerrainBatch *batch =
            s_useLerp ? &s_lerpTerrain[face->drawIndex]
                      : &snapshot->terrain[face->drawIndex];
        const int32_t *cell = batch->cells[face->cellSlot];
        gte = &batch->gte;
        *mirror = batch->mirror;
        tx = (float)(batch->mirror ? -cell[0] : cell[0]);
        ty = (float)cell[1];
        tz = (float)cell[2];
    } else {
        const RageCaptureModelDraw *draw = &snapshot->draws[face->drawIndex];
        gte = s_useLerp ? &s_lerpDrawGte[face->drawIndex] : &draw->gte;
        *mirror = draw->mirror;
        tx = (float)gte->rot.t[0];
        ty = (float)gte->rot.t[1];
        tz = (float)gte->rot.t[2];
    }
    for (vertex = 0; vertex < 4; vertex++) {
        float vx = face->pos[vertex][0];
        float vy = face->pos[vertex][1];
        float vz = face->pos[vertex][2];
        int row;
        for (row = 0; row < 3; row++) {
            float sum = (gte->rot.m[row][0] * vx + gte->rot.m[row][1] * vy +
                         gte->rot.m[row][2] * vz) *
                        (1.0f / 4096.0f);
            out[vertex][row] = sum;
        }
        out[vertex][0] += tx;
        out[vertex][1] += ty;
        out[vertex][2] += tz;
    }
}

static const RageCaptureGteState *ModernFaceGte(
    const RageSceneSnapshot *snapshot, const RageCaptureFace *face) {
    if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        return s_useLerp ? &s_lerpTerrain[face->drawIndex].gte
                         : &snapshot->terrain[face->drawIndex].gte;
    }
    return s_useLerp ? &s_lerpDrawGte[face->drawIndex]
                     : &snapshot->draws[face->drawIndex].gte;
}

static int ModernFaceIsMirror(const RageSceneSnapshot *snapshot,
                              const RageCaptureFace *face) {
    if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        return snapshot->terrain[face->drawIndex].mirror;
    }
    return snapshot->draws[face->drawIndex].mirror ||
           snapshot->draws[face->drawIndex].table != 0;
}

static void ModernBuildFaceVertices(const RageSceneSnapshot *snapshot,
                                    const RageCaptureFace *face,
                                    ModernVertex corners[4],
                                    float *averageZ) {
    const RageCaptureGteState *gte = ModernFaceGte(snapshot, face);
    float view[4][3];
    int mirror = 0;
    int vertex;
    float zSum = 0.0f;
    float windowMin, windowMax;
    float zBias = 0.0f;
    int textured = face->klass == 1 || face->klass == 3;
    /* Depth policy: the compat renderer orders faces by ordering-table
     * bucket (average z quantized by 4<<otShift, plus the per-face bias and
     * the scratch OT base shift). Clamp each vertex's true z into its
     * face's compat bucket window: ordering between faces in different
     * buckets reproduces the oracle exactly (including deliberate biases),
     * while faces meeting inside one bucket still intersect with real
     * per-pixel depth. */
    {
        int bucket = face->otDepth;
        float unit;
        if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
            unit = (float)(4 << snapshot->terrain[face->drawIndex].otShift);
            windowMin = (float)bucket * unit;
            windowMax = windowMin + unit;
        } else if (face->kind == RAGE_CAPTURE_KIND_COURSE) {
            /* The course dispatcher quantizes ((z0>>3)+(z3>>3))>>3 on
             * OTZ-scale (z/4) vertex depths: one bucket is 128 z units,
             * independent of the scratch OT shift. */
            unit = 128.0f;
            bucket += snapshot->draws[face->drawIndex].otBaseBias;
            windowMin = (float)bucket * unit;
            windowMax = windowMin + unit;
        } else {
            /* Model faces (cars, scenery models) use their true per-pixel
             * depth. The per-face bias byte is a painter's overlay order
             * for decals on the model's own surface; translating it into
             * a whole-bucket depth shift let biased faces (wheels, decals)
             * punch through body panels the moment quantized buckets
             * disagreed with the real geometry - retail never shows that.
             * A small epsilon keeps genuinely coplanar overlays ordered. */
            zBias = 4.0f * (float)(face->bias +
                                   snapshot->draws[face->drawIndex]
                                       .otBaseBias);
            windowMin = -1.0e9f;
            windowMax = 1.0e9f;
            (void)bucket;
            (void)unit;
        }
    }
    ModernFaceViewPositions(snapshot, face, view, &mirror);
    if (getenv("RAGE_PORT_MODERN_FACE_TRACE") != NULL) {
        float h = (float)gte->h;
        fprintf(stderr,
                "modern-face kind=%d klass=%d ot=%d bias=%d window=%.0f..%.0f "
                "z=%.0f,%.0f,%.0f,%.0f clut=%04x tpage=%04x "
                "sxy=%.0f,%.0f/%.0f,%.0f/%.0f,%.0f/%.0f,%.0f\n",
                face->kind, face->klass, face->otDepth, face->bias, windowMin,
                windowMax, view[0][2], view[1][2], view[2][2], view[3][2],
                face->clut, face->tpage,
                gte->ofx + view[0][0] * h / (view[0][2] < 1 ? 1 : view[0][2]),
                gte->ofy + view[0][1] * h / (view[0][2] < 1 ? 1 : view[0][2]),
                gte->ofx + view[1][0] * h / (view[1][2] < 1 ? 1 : view[1][2]),
                gte->ofy + view[1][1] * h / (view[1][2] < 1 ? 1 : view[1][2]),
                gte->ofx + view[2][0] * h / (view[2][2] < 1 ? 1 : view[2][2]),
                gte->ofy + view[2][1] * h / (view[2][2] < 1 ? 1 : view[2][2]),
                gte->ofx + view[3][0] * h / (view[3][2] < 1 ? 1 : view[3][2]),
                gte->ofy + view[3][1] * h / (view[3][2] < 1 ? 1 : view[3][2]));
        fprintf(stderr,
                "modern-face-tex window=%05x uv=%02x,%02x/%02x,%02x/"
                "%02x,%02x/%02x,%02x\n",
                face->textureWindow, face->uv[0][0], face->uv[0][1],
                face->uv[1][0], face->uv[1][1], face->uv[2][0],
                face->uv[2][1], face->uv[3][0], face->uv[3][1]);
    }
    for (vertex = 0; vertex < 4; vertex++) {
        ModernVertex *out = &corners[vertex];
        float x = view[vertex][0];
        float y = view[vertex][1];
        float z = view[vertex][2];
        float depthZ;
        float h = (float)gte->h;
        /* w is the true view z, negative behind the camera: the GPU's
         * homogeneous clipping then performs the near clip the compat path
         * approximates with GTE saturation. */
        depthZ = z + zBias;
        if (depthZ < windowMin) depthZ = windowMin;
        if (depthZ > windowMax) depthZ = windowMax;
        zSum += z < 1.0f ? 1.0f : z;
        {
            float halfW = s_logicalW * 0.5f;
            out->x = (z * ((float)gte->ofx - 160.0f) + x * h) / halfW;
            out->y = -(z * ((float)gte->ofy / 120.0f - 1.0f) + y * h / 120.0f);
        }
        out->z = ((depthZ - MODERN_DEPTH_MIN) / MODERN_DEPTH_RANGE) * z;
        out->w = z;
        out->u = (float)face->uv[vertex][0];
        out->v = (float)face->uv[vertex][1];
        out->color[0] = face->color[vertex][0];
        out->color[1] = face->color[vertex][1];
        out->color[2] = face->color[vertex][2];
        out->color[3] =
            (uint8_t)((face->flags & RAGE_CAPTURE_FACE_SEMI) ? 0 : 255);
        out->attr = textured ? face->tpage : (face->tpage | 0x8000u);
        if (getenv("RAGE_PORT_MODERN_SOLID") != NULL) {
            out->attr |= 0x8000u;
        }
        out->twin = face->textureWindow != 0
                        ? ModernTwinFromE2(face->textureWindow)
                        : 0x0000FFFFu;
        out->clut = face->clut;
    }
    *averageZ = zSum * 0.25f;
}

/* ---- 2D packet replay ---- */

static void ModernOrtho(ModernVertex *out, float px, float py) {
    /* The 2D layer stays 4:3, centered inside a widened target. Edge
     * mapping, not the compat rasterizer's half-pixel centres: at scale
     * factors above 1 the +0.5 convention shifts the whole layer by half a
     * logical pixel, leaving one-line gaps around full-screen masks. */
    out->x = (px + s_overscanX) / (s_logicalW * 0.5f) - 1.0f;
    out->y = -(py / 120.0f - 1.0f);
    out->z = 0.0f;
    out->w = 1.0f;
}

static void ModernScissorToPixels(SDL_Rect *rect) {
    float scale = (float)s_targetW / s_logicalW;
    rect->x = (int)(((float)rect->x + s_overscanX) * scale);
    rect->y = rect->y * s_targetH / 240;
    rect->w = (int)((float)rect->w * scale);
    rect->h = rect->h * s_targetH / 240;
}

static void ModernApply2DStateWord(uint32_t word, Modern2DState *state) {
    switch (word >> 24) {
    case 0xE1:
        state->tpage = word & 0x1FFu;
        break;
    case 0xE2:
        state->twin = ModernTwinFromE2(word);
        break;
    case 0xE3: {
        int x = (int)(word & 0x3FFu);
        int y = (int)((word >> 10) & 0x1FFu);
        state->scissor.x = x;
        state->scissor.y = y % 240;
        state->hasScissor = 1;
        break;
    }
    case 0xE4: {
        int x = (int)(word & 0x3FFu);
        int y = (int)((word >> 10) & 0x1FFu);
        state->scissor.w = x - state->scissor.x + 1;
        state->scissor.h = (y % 240) - state->scissor.y + 1;
        state->areaEmpty = state->scissor.w <= 0 || state->scissor.h <= 0;
        break;
    }
    case 0xE5: {
        int x = (int)(word & 0x7FFu);
        int y = (int)((word >> 11) & 0x7FFu);
        state->offsetX = (x ^ 1024) - 1024;
        state->offsetY = (y ^ 1024) - 1024;
        break;
    }
    default:
        break;
    }
}

static void ModernReplay2DPacket(const RageCapturePacket *packet,
                                 Modern2DState *state, int pipelineBase) {
    const uint32_t *words = packet->words;
    uint32_t command = words[0] >> 24;
    if (command >= 0xE0) {
        /* Environment packets (DR_MODE, DR_TWIN, DR_AREA...) carry one
         * E-command per word; apply them all. */
        int word;
        for (word = 0; word < packet->size; word++) {
            ModernApply2DStateWord(words[word], state);
        }
        return;
    }
    if (state->areaEmpty) return;
    {
        int isPoly = (command & 0xE0u) == 0x20u;
        int isLine = (command & 0xE0u) == 0x40u;
        int isRect = (command & 0xE0u) == 0x60u;
        int textured = (command & 0x04u) != 0;
        int gouraud = (command & 0x10u) != 0;
        int quad = (command & 0x08u) != 0;
        int semi = (command & 0x02u) != 0;
        int raw = (command & 0x01u) != 0;
        Modern2DState spanState = *state;
        /* The overlay renders at 320x240 logical coordinates; scale the
         * scissor to the target. */
        if (spanState.hasScissor) {
            ModernScissorToPixels(&spanState.scissor);
            if (spanState.scissor.w <= 0 || spanState.scissor.h <= 0) return;
        }
        if (isPoly) {
            ModernVertex corners[4];
            uint32_t prim_tpage = state->tpage;
            uint32_t clut = 0;
            int count = quad ? 4 : 3;
            int vertex;
            int cursor = 1; /* word 0 = command + colour 0 */
            uint32_t colors[4];
            float rawX[4], rawY[4];
            int minX = 4096, minY = 4096, maxX = -4096, maxY = -4096;
            colors[0] = words[0] & 0xFFFFFFu;
            for (vertex = 0; vertex < count; vertex++) {
                uint32_t xy;
                if (vertex > 0) {
                    if (gouraud) colors[vertex] = words[cursor++] & 0xFFFFFFu;
                    else colors[vertex] = colors[0];
                }
                xy = words[cursor++];
                {
                    int px = (int)((xy & 0x7FFu) ^ 1024) - 1024 +
                             state->offsetX;
                    int py = (int)(((xy >> 16) & 0x7FFu) ^ 1024) - 1024 +
                             state->offsetY;
                    rawX[vertex] = (float)px;
                    rawY[vertex] = (float)py;
                    if (px < minX) minX = px;
                    if (px > maxX) maxX = px;
                    if (py < minY) minY = py;
                    if (py > maxY) maxY = py;
                }
                memset(&corners[vertex], 0, sizeof(corners[vertex]));
                if (textured) {
                    uint32_t uvWord = words[cursor++];
                    corners[vertex].u = (float)(uvWord & 0xFFu);
                    corners[vertex].v = (float)((uvWord >> 8) & 0xFFu);
                    if (vertex == 0) clut = (uvWord >> 16) & 0xFFFFu;
                    if (vertex == 1) prim_tpage = (uvWord >> 16) & 0x1FFu;
                }
            }
            /* Full-screen overlays (fades, night filters) stretch across a
             * widened view; otherwise they mask only the 4:3 centre. */
            if (s_overscanX > 0.0f && minX <= 0 && minY <= 0 && maxX >= 320 &&
                maxY >= 240) {
                for (vertex = 0; vertex < count; vertex++) {
                    if (rawX[vertex] <= 0.0f) rawX[vertex] = -s_overscanX;
                    else if (rawX[vertex] >= 320.0f)
                        rawX[vertex] = 320.0f + s_overscanX;
                }
            }
            for (vertex = 0; vertex < count; vertex++) {
                ModernOrtho(&corners[vertex], rawX[vertex], rawY[vertex]);
            }
            for (vertex = 0; vertex < count; vertex++) {
                ModernVertex *out = &corners[vertex];
                uint32_t color = raw && textured ? 0x808080u : colors[vertex];
                out->color[0] = (uint8_t)(color & 0xFFu);
                out->color[1] = (uint8_t)((color >> 8) & 0xFFu);
                out->color[2] = (uint8_t)((color >> 16) & 0xFFu);
                out->color[3] = (uint8_t)(semi ? 0 : 255);
                out->attr = textured ? prim_tpage : (prim_tpage | 0x8000u);
                out->twin = state->twin;
                out->clut = clut;
            }
            {
                uint32_t abr = (prim_tpage >> 5) & 3u;
                int pipeline = (semi && abr == 2u) ? pipelineBase + 1
                                                   : pipelineBase;
                ModernSpan *span = ModernBeginSpan(pipeline, &spanState, 0.0f);
                if (quad) ModernEmitQuad(span, corners);
                else ModernEmitTriangle(span, corners);
            }
        } else if (isRect) {
            ModernVertex corners[4];
            int sizeMode = (int)((command >> 3) & 3u);
            int cursor = 1;
            uint32_t xy, uvWord = 0, clut = 0;
            int px, py, w, h;
            int u0 = 0, v0 = 0;
            xy = words[cursor++];
            px = (int)((xy & 0x7FFu) ^ 1024) - 1024;
            py = (int)(((xy >> 16) & 0x7FFu) ^ 1024) - 1024;
            if (textured) {
                uvWord = words[cursor++];
                u0 = (int)(uvWord & 0xFFu);
                v0 = (int)((uvWord >> 8) & 0xFFu);
                clut = (uvWord >> 16) & 0xFFFFu;
            }
            if (sizeMode == 0) {
                uint32_t wh = words[cursor++];
                w = (int)(wh & 0x3FFu);
                h = (int)((wh >> 16) & 0x1FFu);
            } else if (sizeMode == 1) {
                w = h = 1;
            } else if (sizeMode == 2) {
                w = h = 8;
            } else {
                w = h = 16;
            }
            px += state->offsetX;
            py += state->offsetY;
            if (s_overscanX > 0.0f && !textured && px <= 0 && py <= 0 &&
                px + w >= 320 && py + h >= 240) {
                /* Full-screen fade/filter tiles cover the widened view. */
                px = (int)(-s_overscanX - 1.0f);
                w = 320 - 2 * px;
            }
            {
                int vertex;
                for (vertex = 0; vertex < 4; vertex++) {
                    ModernVertex *out = &corners[vertex];
                    int right = vertex & 1;
                    int bottom = vertex >> 1;
                    memset(out, 0, sizeof(*out));
                    ModernOrtho(out, (float)(px + (right ? w : 0)),
                                (float)(py + (bottom ? h : 0)));
                    out->u = (float)(u0 + (right ? w : 0));
                    out->v = (float)(v0 + (bottom ? h : 0));
                    {
                        uint32_t color =
                            raw && textured ? 0x808080u : (words[0] & 0xFFFFFFu);
                        out->color[0] = (uint8_t)(color & 0xFFu);
                        out->color[1] = (uint8_t)((color >> 8) & 0xFFu);
                        out->color[2] = (uint8_t)((color >> 16) & 0xFFu);
                        out->color[3] = (uint8_t)(semi ? 0 : 255);
                    }
                    out->attr =
                        textured ? state->tpage : (state->tpage | 0x8000u);
                    out->twin = state->twin;
                    out->clut = clut;
                }
            }
            {
                uint32_t abr = (state->tpage >> 5) & 3u;
                int pipeline = (semi && abr == 2u) ? pipelineBase + 1
                                                   : pipelineBase;
                ModernSpan *span = ModernBeginSpan(pipeline, &spanState, 0.0f);
                ModernEmitQuad(span, corners);
            }
        } else if (isLine) {
            /* Fixed two/three-point lines; expand each segment to a thin
             * quad, PS1-style single-pixel thickness. */
            int points[8][2];
            uint32_t colors[8];
            int count = 0;
            int cursor = 1;
            int vertex;
            colors[0] = words[0] & 0xFFFFFFu;
            while (cursor < packet->size && count < 8) {
                uint32_t word;
                if (count > 0 && gouraud) {
                    if (cursor >= packet->size) break;
                    colors[count] = words[cursor++] & 0xFFFFFFu;
                }
                if (cursor >= packet->size) break;
                word = words[cursor++];
                if ((word & 0xF000F000u) == 0x50005000u && count >= 2) break;
                points[count][0] =
                    (int)((word & 0x7FFu) ^ 1024) - 1024 + state->offsetX;
                points[count][1] =
                    (int)(((word >> 16) & 0x7FFu) ^ 1024) - 1024 +
                    state->offsetY;
                if (count > 0 && !gouraud) colors[count] = colors[0];
                count++;
            }
            for (vertex = 0; vertex + 1 < count; vertex++) {
                ModernVertex corners[4];
                int x0 = points[vertex][0], y0 = points[vertex][1];
                int x1 = points[vertex + 1][0], y1 = points[vertex + 1][1];
                int horizontal = abs(x1 - x0) >= abs(y1 - y0);
                int corner;
                for (corner = 0; corner < 4; corner++) {
                    ModernVertex *out = &corners[corner];
                    int end = corner & 1;
                    int side = corner >> 1;
                    float px = (float)(end ? x1 : x0);
                    float py = (float)(end ? y1 : y0);
                    if (horizontal) py += side;
                    else px += side;
                    memset(out, 0, sizeof(*out));
                    ModernOrtho(out, px, py);
                    {
                        uint32_t color = colors[end ? vertex + 1 : vertex];
                        out->color[0] = (uint8_t)(color & 0xFFu);
                        out->color[1] = (uint8_t)((color >> 8) & 0xFFu);
                        out->color[2] = (uint8_t)((color >> 16) & 0xFFu);
                        out->color[3] = (uint8_t)(semi ? 0 : 255);
                    }
                    out->attr = state->tpage | 0x8000u;
                    out->twin = state->twin;
                    out->clut = 0;
                }
                {
                    ModernSpan *span =
                        ModernBeginSpan(pipelineBase, &spanState, 0.0f);
                    ModernEmitQuad(span, corners);
                }
            }
        }
    }
}

/* ---- snapshot -> vertex/span lists ---- */

static void ModernBuildFrame(const RageSceneSnapshot *snapshot) {
    Modern2DState state2d;
    int i;
    static ModernFaceOrder semiOrder[RAGE_CAPTURE_MAX_FACES];
    int semiCount = 0;

    s_vertexCount = 0;
    s_spanCount = 0;
    memset(&state2d, 0, sizeof(state2d));
    state2d.twin = 0x0000FFFFu;

    /* Background 2D: sky layers at the far ordering-table buckets. */
    for (i = 0; i < snapshot->packetCount; i++) {
        const RageCapturePacket *packet = &snapshot->packets[i];
        if (packet->table != 0) continue;
        if (packet->bucket < MODERN_BACKGROUND_BUCKET &&
            (packet->words[0] >> 24) < 0xE0u) {
            continue;
        }
        /* State packets always replay so tpage/window state stays right for
         * the foreground pass below; draw packets only in far buckets. */
        if (packet->bucket >= MODERN_BACKGROUND_BUCKET ||
            (packet->words[0] >> 24) >= 0xE0u) {
            ModernReplay2DPacket(packet, &state2d, MODERN_PIPE_2D);
        }
    }

    /* Opaque 3D, z-buffered. Drawn in reverse submission order: within a
     * compat ordering-table bucket AddPrim prepends, so the first-submitted
     * face is drawn last and wins; LESS_OR_EQUAL plus reverse order keeps
     * that rule for equal-depth (coplanar) faces. */
    for (i = snapshot->faceCount - 1; i >= 0; i--) {
        const RageCaptureFace *face = &snapshot->faces[i];
        ModernVertex corners[4];
        float averageZ;
        if (ModernFaceIsMirror(snapshot, face)) continue;
        if (face->flags & RAGE_CAPTURE_FACE_SEMI) continue;
        ModernBuildFaceVertices(snapshot, face, corners, &averageZ);
        ModernEmitQuad(ModernBeginSpan(MODERN_PIPE_3D_OPAQUE, NULL, 0.0f),
                       corners);
    }
    for (i = 0; i < snapshot->faceCount; i++) {
        const RageCaptureFace *face = &snapshot->faces[i];
        if (ModernFaceIsMirror(snapshot, face)) continue;
        if (!(face->flags & RAGE_CAPTURE_FACE_SEMI)) continue;
        if (semiCount < RAGE_CAPTURE_MAX_FACES) {
            semiOrder[semiCount].index = i;
            semiOrder[semiCount].depth = 0.0f;
            semiCount++;
        }
    }

    /* Semi-transparent 3D, back to front. */
    for (i = 0; i < semiCount; i++) {
        const RageCaptureFace *face = &snapshot->faces[semiOrder[i].index];
        ModernVertex corners[4];
        float averageZ;
        ModernBuildFaceVertices(snapshot, face, corners, &averageZ);
        semiOrder[i].depth = averageZ;
    }
    if (semiCount > 1) {
        qsort(semiOrder, semiCount, sizeof(semiOrder[0]),
              ModernCompareFaceOrder);
    }
    for (i = 0; i < semiCount; i++) {
        const RageCaptureFace *face = &snapshot->faces[semiOrder[i].index];
        ModernVertex corners[4];
        float averageZ;
        uint32_t abr = (face->tpage >> 5) & 3u;
        int pipeline =
            abr == 2u ? MODERN_PIPE_3D_SUB : MODERN_PIPE_3D_BLEND;
        ModernBuildFaceVertices(snapshot, face, corners, &averageZ);
        ModernEmitQuad(ModernBeginSpan(pipeline, NULL, (float)i), corners);
    }

    /* Foreground 2D of the main ordering table, in compat draw order. */
    memset(&state2d, 0, sizeof(state2d));
    state2d.twin = 0x0000FFFFu;
    for (i = 0; i < snapshot->packetCount; i++) {
        const RageCapturePacket *packet = &snapshot->packets[i];
        uint32_t command = packet->words[0] >> 24;
        if (packet->table != 0) continue;
        if (packet->bucket >= MODERN_BACKGROUND_BUCKET && command < 0xE0u) {
            continue;
        }
        ModernReplay2DPacket(packet, &state2d, MODERN_PIPE_2D);
    }
    /* The mirror table inherits the drawing-area state the main chain left
     * behind; keep it for the mirror sections below. */
    s_mainEnd2D = state2d;

    /* Mirror overlay: the second ordering table draws after the main scene
     * and INHERITS the GPU drawing-area state the main table's chain left
     * behind - that inherited area is how retail suppresses the mirror
     * content while the panel slides in (DrawMirrorFrame's partial or
     * zero-height area sits at the end of the main chain). The mirror 3D is
     * scissored to the intersection of the inherited area and the mirror
     * stream's own area. */
    s_currentPass = 1;
    {
        SDL_Rect area;
        int haveArea = 0;
        Modern2DState scan = s_mainEnd2D;
        int inheritedEmpty =
            s_mainEnd2D.hasScissor &&
            (s_mainEnd2D.areaEmpty || s_mainEnd2D.scissor.w <= 0 ||
             s_mainEnd2D.scissor.h <= 0);
        for (i = 0; i < snapshot->packetCount; i++) {
            const RageCapturePacket *packet = &snapshot->packets[i];
            int word;
            if (packet->table != 1) continue;
            if ((packet->words[0] >> 24) < 0xE0u) continue;
            for (word = 0; word < packet->size; word++) {
                ModernApply2DStateWord(packet->words[word], &scan);
            }
            if (scan.hasScissor && scan.scissor.w != 0) {
                break;
            }
        }
        haveArea = scan.hasScissor && !scan.areaEmpty && scan.scissor.w > 0 &&
                   scan.scissor.h > 0 && !inheritedEmpty;
        area = scan.scissor;
        if (haveArea && s_mainEnd2D.hasScissor && !inheritedEmpty) {
            int x0 = area.x > s_mainEnd2D.scissor.x ? area.x : s_mainEnd2D.scissor.x;
            int y0 = area.y > s_mainEnd2D.scissor.y ? area.y : s_mainEnd2D.scissor.y;
            int x1 = area.x + area.w < s_mainEnd2D.scissor.x + s_mainEnd2D.scissor.w
                         ? area.x + area.w
                         : s_mainEnd2D.scissor.x + s_mainEnd2D.scissor.w;
            int y1 = area.y + area.h < s_mainEnd2D.scissor.y + s_mainEnd2D.scissor.h
                         ? area.y + area.h
                         : s_mainEnd2D.scissor.y + s_mainEnd2D.scissor.h;
            area.x = x0;
            area.y = y0;
            area.w = x1 - x0;
            area.h = y1 - y0;
            if (area.w <= 0 || area.h <= 0) haveArea = 0;
        }
        /* Mirror backdrop 2D (sky bands at far buckets) before the 3D,
         * starting from the inherited main-table state so slide-in frames
         * clip exactly like retail. */
        state2d = s_mainEnd2D;
        for (i = 0; i < snapshot->packetCount; i++) {
            const RageCapturePacket *packet = &snapshot->packets[i];
            uint32_t command = packet->words[0] >> 24;
            if (packet->table != 1) continue;
            if (command < 0xE0u &&
                (!state2d.hasScissor ||
                 packet->bucket < MODERN_BACKGROUND_BUCKET)) {
                continue;
            }
            ModernReplay2DPacket(packet, &state2d, MODERN_PIPE_2D);
        }
        if (haveArea) {
            Modern2DState scissorState;
            memset(&scissorState, 0, sizeof(scissorState));
            scissorState.hasScissor = 1;
            scissorState.scissor = area;
            ModernScissorToPixels(&scissorState.scissor);
            for (i = snapshot->faceCount - 1; i >= 0; i--) {
                const RageCaptureFace *face = &snapshot->faces[i];
                ModernVertex corners[4];
                float averageZ;
                if (!ModernFaceIsMirror(snapshot, face)) continue;
                if (face->flags & RAGE_CAPTURE_FACE_SEMI) continue;
                ModernBuildFaceVertices(snapshot, face, corners, &averageZ);
                ModernEmitQuad(ModernBeginSpan(MODERN_PIPE_3D_OPAQUE,
                                               &scissorState, 0.0f),
                               corners);
            }
            for (i = 0; i < snapshot->faceCount; i++) {
                const RageCaptureFace *face = &snapshot->faces[i];
                ModernVertex corners[4];
                float averageZ;
                if (!ModernFaceIsMirror(snapshot, face)) continue;
                if (!(face->flags & RAGE_CAPTURE_FACE_SEMI)) continue;
                ModernBuildFaceVertices(snapshot, face, corners, &averageZ);
                ModernEmitQuad(ModernBeginSpan(MODERN_PIPE_3D_BLEND,
                                               &scissorState, 0.0f),
                               corners);
            }
        }
    }

    /* Mirror-table foreground 2D (frame border, text), in compat order;
     * starts from the inherited main-table state like the backdrop. */
    state2d = s_mainEnd2D;
    for (i = 0; i < snapshot->packetCount; i++) {
        const RageCapturePacket *packet = &snapshot->packets[i];
        uint32_t command = packet->words[0] >> 24;
        if (packet->table != 1) continue;
        if (command < 0xE0u &&
            (!state2d.hasScissor ||
             packet->bucket >= MODERN_BACKGROUND_BUCKET)) {
            continue;
        }
        ModernReplay2DPacket(packet, &state2d, MODERN_PIPE_2D);
    }
    s_currentPass = 0;
}

/* ---- rendering ---- */

static void ModernRender(const RageSceneSnapshot *snapshot) {
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUTexture *vram = Psyz_VideoGetVramTexture_SDL3GPU();
    int i;
    if (vram == NULL) return;
    ModernBuildFrame(snapshot);
    cmd = SDL_AcquireGPUCommandBuffer(s_device);
    if (cmd == NULL) return;
    if (s_vertexCount > 0) {
        void *mapped = SDL_MapGPUTransferBuffer(s_device, s_vertexTransfer,
                                                true);
        if (mapped == NULL) {
            SDL_CancelGPUCommandBuffer(cmd);
            return;
        }
        memcpy(mapped, s_vertices, s_vertexCount * sizeof(ModernVertex));
        SDL_UnmapGPUTransferBuffer(s_device, s_vertexTransfer);
        {
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
            const SDL_GPUTransferBufferLocation source = {
                .transfer_buffer = s_vertexTransfer, .offset = 0};
            const SDL_GPUBufferRegion destination = {
                .buffer = s_vertexBuffer,
                .offset = 0,
                .size = (Uint32)(s_vertexCount * sizeof(ModernVertex))};
            SDL_UploadToGPUBuffer(copy, &source, &destination, true);
            SDL_EndGPUCopyPass(copy);
        }
    }
    {
        SDL_GPUGraphicsPipeline *pipelines[5];
        int spanIndex = 0;
        int passNumber;
        pipelines[MODERN_PIPE_3D_OPAQUE] = s_pipe3dOpaque;
        pipelines[MODERN_PIPE_3D_BLEND] = s_pipe3dBlend;
        pipelines[MODERN_PIPE_3D_SUB] = s_pipe3dSub;
        pipelines[MODERN_PIPE_2D] = s_pipe2d;
        pipelines[MODERN_PIPE_2D_SUB] = s_pipe2dSub;
        /* Pass 0 clears colour and depth and draws the main scene; pass 1
         * reclears only depth and draws the mirror overlay over it. */
        for (passNumber = 0; passNumber < 2; passNumber++) {
            SDL_GPUColorTargetInfo color = {
                .texture = s_target,
                .clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
                .load_op = passNumber == 0 ? SDL_GPU_LOADOP_CLEAR
                                           : SDL_GPU_LOADOP_LOAD,
                .store_op = SDL_GPU_STOREOP_STORE,
            };
            SDL_GPUDepthStencilTargetInfo depth = {
                .texture = s_depth,
                .clear_depth = 1.0f,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_DONT_CARE,
            };
            SDL_GPURenderPass *pass;
            SDL_GPUGraphicsPipeline *bound = NULL;
            if (passNumber == 1 && spanIndex >= s_spanCount) break;
            pass = SDL_BeginGPURenderPass(cmd, &color, 1, &depth);
            if (pass == NULL) break;
            if (s_vertexCount > 0) {
                const SDL_GPUBufferBinding binding = {
                    .buffer = s_vertexBuffer, .offset = 0};
                const SDL_GPUTextureSamplerBinding sampler = {
                    .texture = vram, .sampler = s_sampler};
                SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
                SDL_BindGPUFragmentSamplers(pass, 0, &sampler, 1);
                for (; spanIndex < s_spanCount; spanIndex++) {
                    const ModernSpan *span = &s_spans[spanIndex];
                    if (span->pass != passNumber) break;
                    if (span->count == 0) continue;
                    if (pipelines[span->pipeline] != bound) {
                        bound = pipelines[span->pipeline];
                        SDL_BindGPUGraphicsPipeline(pass, bound);
                    }
                    {
                        SDL_Rect scissor = {0, 0, s_targetW, s_targetH};
                        if (span->hasScissor) scissor = span->scissor;
                        SDL_SetGPUScissor(pass, &scissor);
                    }
                    SDL_DrawGPUPrimitives(pass, (Uint32)span->count, 1,
                                          (Uint32)span->start, 0);
                }
            }
            SDL_EndGPURenderPass(pass);
        }
    }
    if (s_config.modernPost != RAGE_MODERN_POST_NONE && s_pipePost != NULL) {
        const SDL_GPUColorTargetInfo color = {
            .texture = s_postTarget,
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &color, 1, NULL);
        if (pass != NULL) {
            const SDL_GPUTextureSamplerBinding sampler = {
                .texture = s_target, .sampler = s_samplerLinear};
            SDL_BindGPUGraphicsPipeline(pass, s_pipePost);
            SDL_BindGPUFragmentSamplers(pass, 0, &sampler, 1);
            SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        }
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    s_haveRenderedFrame = 1;
    if (getenv("RAGE_PORT_MODERN_SPAN_TRACE") != NULL) {
        int counts[5] = {0};
        int verts[5] = {0};
        for (i = 0; i < s_spanCount; i++) {
            counts[s_spans[i].pipeline]++;
            verts[s_spans[i].pipeline] += s_spans[i].count;
        }
        fprintf(stderr,
                "modern-spans frame=%u spans=%d verts=%d "
                "opaque=%d/%d blend=%d/%d sub=%d/%d 2d=%d/%d 2dsub=%d/%d\n",
                snapshot->frameCounter, s_spanCount, s_vertexCount,
                counts[0], verts[0], counts[1], verts[1], counts[2], verts[2],
                counts[3], verts[3], counts[4], verts[4]);
    }
}

/* Diagnostic: write the modern target as a binary PPM when
 * RAGE_PORT_MODERN_DUMP names a path and RAGE_PORT_MODERN_DUMP_FRAME (if
 * set) matches the captured frame counter. */
static void ModernMaybeDump(const RageSceneSnapshot *snapshot) {
    static int initialized;
    static const char *path;
    static long frame = -1;
    static int done;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *cmd;
    if (!initialized) {
        const char *frameText = getenv("RAGE_PORT_MODERN_DUMP_FRAME");
        path = getenv("RAGE_PORT_MODERN_DUMP");
        if (frameText != NULL) frame = strtol(frameText, NULL, 0);
        initialized = 1;
    }
    if (path == NULL || done) return;
    if (frame >= 0 && (long)snapshot->frameCounter < frame) return;
    {
        SDL_GPUTransferBufferCreateInfo info = {0};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        info.size = (Uint32)(s_targetW * s_targetH * 4);
        transfer = SDL_CreateGPUTransferBuffer(s_device, &info);
    }
    if (transfer == NULL) return;
    cmd = SDL_AcquireGPUCommandBuffer(s_device);
    if (cmd != NULL) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        const SDL_GPUTextureRegion region = {
            .texture = s_config.modernPost != RAGE_MODERN_POST_NONE &&
                               s_postTarget != NULL
                           ? s_postTarget
                           : s_target,
            .w = (Uint32)s_targetW,
            .h = (Uint32)s_targetH,
            .d = 1,
        };
        const SDL_GPUTextureTransferInfo destination = {
            .transfer_buffer = transfer,
        };
        SDL_DownloadFromGPUTexture(copy, &region, &destination);
        SDL_EndGPUCopyPass(copy);
        {
            SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
            if (fence != NULL) {
                SDL_WaitForGPUFences(s_device, true, &fence, 1);
                SDL_ReleaseGPUFence(s_device, fence);
            }
        }
        {
            const uint8_t *pixels =
                SDL_MapGPUTransferBuffer(s_device, transfer, false);
            if (pixels != NULL) {
                FILE *file = fopen(path, "wb");
                if (file != NULL) {
                    int i;
                    fprintf(file, "P6\n%d %d\n255\n", s_targetW, s_targetH);
                    for (i = 0; i < s_targetW * s_targetH; i++) {
                        fwrite(pixels + i * 4, 1, 3, file);
                    }
                    fclose(file);
                    fprintf(stderr,
                            "rage-port: modern dump frame=%u -> %s\n",
                            snapshot->frameCounter, path);
                }
                SDL_UnmapGPUTransferBuffer(s_device, transfer);
            }
        }
    }
    SDL_ReleaseGPUTransferBuffer(s_device, transfer);
    done = 1;
}

/* ---- hooks ---- */

static void ModernOverlayInit(SDL_Window *window, SDL_GPUDevice *device) {
    s_window = window;
    s_device = device;
    if (s_prev_overlay_init) {
        s_prev_overlay_init(window, device);
    }
}

static void ModernPresentSource(PsyzPresentSourceInfo *info) {
    const RageSceneSnapshot *snapshot;
    int fpsMode;
    if (s_prev_present_source) {
        s_prev_present_source(info);
    }
    if (!s_enabled || s_device == NULL) return;
    fpsMode = s_config.modernFps != RAGE_MODERN_FPS_LOGIC;
    /* Both modes render the PREVIOUS logic frame - the one compat is
     * presenting during this tick; fps mode moves its transforms toward
     * the newest frame by the wall-clock fraction of the tick. */
    snapshot = RageCapturePrevious();
    /* Scenes with no captured 3D pass through to the compat image, and so
     * do 480-line menu scenes: their double-height buffer follows PS1
     * interlace conventions the compat presenter already handles. */
    if (snapshot->faceCount == 0) return;
    if (snapshot->displayHeight != 0 && snapshot->displayHeight != 240) {
        return;
    }
    if (!ModernEnsureResources()) return;
    if (fpsMode) {
        const RageSceneSnapshot *target = RageCaptureCurrent();
        Uint64 now = SDL_GetTicksNS();
        float t = 1.0f;
        if (target->frameCounter != s_tickFrame) {
            if (s_tickTimeNs != 0) {
                Uint64 delta = now - s_tickTimeNs;
                if (delta > 1000000 && delta < 200000000) {
                    s_tickIntervalNs = delta;
                }
            }
            s_tickFrame = target->frameCounter;
            s_tickTimeNs = now;
        }
        if (s_tickIntervalNs > 0) {
            double fraction = (double)(now - s_tickTimeNs) /
                              (double)s_tickIntervalNs;
            t = fraction >= 1.0 ? 1.0f : (float)fraction;
        }
        ModernPrepareInterpolation(snapshot, target, t);
        ModernRender(snapshot);
        s_useLerp = 0;
        if (s_haveRenderedFrame) ModernMaybeDump(snapshot);
    } else if (snapshot->frameCounter != s_lastRenderedFrame) {
        ModernRender(snapshot);
        s_lastRenderedFrame = snapshot->frameCounter;
        if (s_haveRenderedFrame) ModernMaybeDump(snapshot);
    }
    if (!s_haveRenderedFrame) return;
    info->texture = s_config.modernPost != RAGE_MODERN_POST_NONE &&
                            s_postTarget != NULL
                        ? s_postTarget
                        : s_target;
    info->w = (Uint32)s_targetW;
    info->h = (Uint32)s_targetH;
    info->aspect = (4.0f / 3.0f) * (s_logicalW / 320.0f);
    info->filter = s_config.modernTextureFilterLinear
                       ? SDL_GPU_FILTER_LINEAR
                       : SDL_GPU_FILTER_NEAREST;
}

/* Called by the main loop's frame-sync wait. When an FPS mode is selected,
 * present additional interpolated frames while race logic (threshold 0x180,
 * one tick per two VBlanks) waits out its interval; menu-rate scenes already
 * present every VBlank. Calling the platform present directly skips the
 * BIOS pad refresh, so input edge semantics are untouched. */
void RageModernFrameWaitTick(int frameLimit) {
    if (!s_enabled || s_device == NULL) return;
    if (s_config.modernFps == RAGE_MODERN_FPS_LOGIC) return;
    if (frameLimit < 0x180) return;
    if (RageCaptureCurrent()->faceCount == 0) return;
    /* Stop presenting when less than a whole VBlank remains: an
     * intermediate present here consumes the swapchain image the game's
     * own VSync(0) present is about to wait for, stretching the logic
     * tick by a display refresh. */
    if (Psyz_VideoVSync(1) + 0x100 > frameLimit) return;
    {
        /* Explicit targets pace to 1/fps. Vsync mode paces to the actual
         * display refresh: presenting faster only queues swapchain images
         * the game's own VSync(0) present then has to wait out, which
         * stretches the race tick (observed as cars at half speed while
         * the VBlank-derived clock stayed correct). */
        static Uint64 lastPresentNs;
        static Uint64 displayIntervalNs;
        Uint64 now = SDL_GetTicksNS();
        Uint64 interval;
        if (s_config.modernFps > 0) {
            interval = (Uint64)(1000000000.0 / s_config.modernFps);
        } else {
            if (displayIntervalNs == 0) {
                const SDL_DisplayMode *mode =
                    s_window != NULL
                        ? SDL_GetCurrentDisplayMode(
                              SDL_GetDisplayForWindow(s_window))
                        : NULL;
                displayIntervalNs =
                    mode != NULL && mode->refresh_rate > 1.0f
                        ? (Uint64)(1000000000.0 / mode->refresh_rate)
                        : 16666667u;
            }
            interval = displayIntervalNs;
        }
        if (now - lastPresentNs < interval) return;
        lastPresentNs = now;
    }
    Psyz_VideoPresentIntermediate();
}

int RageModernInit(const RagePortConfig *config) {
    if (config->renderer != RAGE_RENDERER_MODERN) {
        return 1;
    }
    if (s_enabled) {
        return 1;
    }
    s_config = *config;
    s_prev_overlay_init = Psyz_OverlayInit_SDL3GPU(ModernOverlayInit);
    s_prev_present_source = Psyz_PresentSource_SDL3GPU(ModernPresentSource);
    s_enabled = 1;
    fprintf(stderr, "rage-port: modern renderer enabled (scale %.2f)\n",
            s_config.modernInternalScale);
    return 1;
}

void RageModernShutdown(void) {
    if (!s_enabled) {
        return;
    }
    Psyz_PresentSource_SDL3GPU(s_prev_present_source);
    Psyz_OverlayInit_SDL3GPU(s_prev_overlay_init);
    s_prev_present_source = NULL;
    s_prev_overlay_init = NULL;
    s_window = NULL;
    s_device = NULL;
    s_enabled = 0;
}

int RageModernIsEnabled(void) {
    return s_enabled;
}

int RageModernCullMarginX(void) {
    if (!s_enabled || s_config.modernAspect != RAGE_MODERN_ASPECT_16_9) {
        return 0;
    }
    /* Half the widened logical width, rounded up: 320*(16/9)/(4/3) adds
     * ~53.3 columns per side. */
    return 54;
}

int RageModernExtendedDepth(void) {
    return s_enabled;
}
