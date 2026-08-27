#include "modern_renderer.h"
#include "modern_assets.h"
#include "modern_native_gpu.h"
#include "rage/render_world_game.h"

#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>
#include <psyz/video.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene_capture.h"
#include "modern_texture_dump.h"
#include "../platform_paths.h"
#include "game/course_index.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "modern_renderer_diagnostics.h"
#include "../runtime_config.h"
#include "shaders/modern_vert_spv.h"
#include "shaders/modern_frag_spv.h"
#include "shaders/post_vert_spv.h"
#include "shaders/post_frag_spv.h"

/* The modern presentation path combines the native RenderWorld renderer,
 * including its sky, with captured PS1 2D layers that still own the HUD and
 * mirror frame. Captured PS1 3D faces and sky packets are never rendered
 * here. Menus, FMV and 480-line screens present the compat image directly. */

static int s_enabled;
static int s_initialized;
static SDL_Scancode s_toggleScancode = SDL_SCANCODE_F10;
static int s_toggleWasDown;
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
static SDL_GPUTexture *s_mirrorTarget;
static SDL_GPUTexture *s_mirrorDepth;
static int s_mirrorTargetW, s_mirrorTargetH;
static SDL_GPUSampler *s_sampler;
static SDL_GPUGraphicsPipeline *s_pipe2d;
static SDL_GPUGraphicsPipeline *s_pipe2dSub;
static SDL_GPUBuffer *s_vertexBuffer;
static SDL_GPUTransferBuffer *s_vertexTransfer;
static SDL_GPUTexture *s_postTarget;
static SDL_GPUGraphicsPipeline *s_pipePost;
static SDL_GPUSampler *s_samplerLinear;

/* Ring of recent presented frames, copied on the GPU every present so a
 * transient artifact can be dumped AFTER being seen (the synchronous burst
 * capture stalls presentation and suppresses timing-dependent flashes). */
#define MODERN_RING 16
static SDL_GPUTexture *s_ring[MODERN_RING];
static uint32_t s_ringFrame[MODERN_RING];
static float s_ringT[MODERN_RING];
static int s_ringNext;
static RageSceneSnapshot *s_ringScene; /* MODERN_RING copies */
static int s_ringEnabled;
static int s_resourcesReady;
static unsigned int s_resourceGeneration;
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
    MODERN_PIPE_2D,
    MODERN_PIPE_2D_SUB,
};

typedef struct ModernSpan {
    uint8_t pipeline;
    uint8_t hasScissor;
    uint8_t pass; /* 0 = main scene, 1 = mirror overlay (depth recleared) */
    uint8_t layer;
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
#define MODERN_DEPTH_RANGE (240000.0f)

/* ---- MSL shaders ---- */

#include "modern_shader_sources.h"
static SDL_GPUTexture *s_finalTarget;
static SDL_GPUGraphicsPipeline *s_pipeComposite;

/* ---- resource creation ---- */

static SDL_GPUShader *ModernCreateShader(
    const unsigned char *spirv, size_t spirvSize,
    const char *msl, size_t mslSize, SDL_GPUShaderStage stage,
    const char *mslEntry, int samplers) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(s_device);
    SDL_GPUShaderCreateInfo info = {0};
    info.stage = stage;
    info.num_samplers = (Uint32)samplers;
    if ((formats & SDL_GPU_SHADERFORMAT_SPIRV) && spirv != NULL) {
        info.code = spirv;
        info.code_size = spirvSize;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if ((formats & SDL_GPU_SHADERFORMAT_MSL) && msl != NULL) {
        info.code = (const Uint8 *)msl;
        info.code_size = mslSize;
        info.entrypoint = mslEntry;
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(s_device, &info);
}

static SDL_GPUGraphicsPipeline *ModernCreateFullscreenPipeline(
    const unsigned char *vsSpirv, size_t vsSpirvSize,
    const char *vsMsl, size_t vsMslSize, const char *vsMslEntry,
    const unsigned char *fsSpirv, size_t fsSpirvSize,
    const char *fsMsl, size_t fsMslSize, const char *fsMslEntry, int samplers) {
    SDL_GPUShader *vs = ModernCreateShader(
        vsSpirv, vsSpirvSize, vsMsl, vsMslSize, SDL_GPU_SHADERSTAGE_VERTEX,
        vsMslEntry, 0);
    SDL_GPUShader *fs = ModernCreateShader(
        fsSpirv, fsSpirvSize, fsMsl, fsMslSize, SDL_GPU_SHADERSTAGE_FRAGMENT,
        fsMslEntry, samplers);
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

static SDL_GPUGraphicsPipeline *ModernCreatePostPipeline(void) {
    return ModernCreateFullscreenPipeline(
        post_vert_spv, post_vert_spv_len,
        MODERN_POST_MSL, MODERN_POST_MSL_SIZE, "vs_post",
        post_frag_spv, post_frag_spv_len,
        MODERN_POST_MSL, MODERN_POST_MSL_SIZE, "fs_post", 1);
}

static SDL_GPUGraphicsPipeline *ModernCreateCompositePipeline(void) {
    char header[128];
    size_t headerLen;
    char *source;
    size_t sourceSize;
    SDL_GPUGraphicsPipeline *pipeline;
    snprintf(header, sizeof(header),
             "constant int kGrading = %d;\n", s_config.modernGrading);
    headerLen = strlen(header);
    sourceSize = MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1 + headerLen +
                 MODERN_COMPOSITE_MSL_SIZE;
    source = malloc(sourceSize);
    if (source == NULL) return NULL;
    memcpy(source, MODERN_COMPOSITE_PROLOGUE_MSL,
           MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1);
    memcpy(source + MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1, header,
           headerLen);
    memcpy(source + MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1 + headerLen,
           MODERN_COMPOSITE_MSL, MODERN_COMPOSITE_MSL_SIZE);
    pipeline = ModernCreateFullscreenPipeline(
        NULL, 0, MODERN_EFFECTS_MSL, MODERN_EFFECTS_MSL_SIZE, "vs_fx",
        NULL, 0, source, sourceSize, "fs_composite", 1);
    free(source);
    return pipeline;
}

static SDL_GPUGraphicsPipeline *ModernCreateOverlayPipeline(
    SDL_GPUShader *vs, SDL_GPUShader *fs, int subtract) {
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
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = false;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(s_device, &info);
}

static void ModernDestroyResources(void) {
    int slot;
    int hadResources = s_resourcesReady;
    if (s_device != NULL) {
#define RAGE_RELEASE(kind, value) do {                                        \
        if ((value) != NULL) SDL_ReleaseGPU##kind(s_device, (value));          \
        (value) = NULL;                                                        \
    } while (0)
        RAGE_RELEASE(Texture, s_target);
        RAGE_RELEASE(Texture, s_depth);
        RAGE_RELEASE(Texture, s_mirrorTarget);
        RAGE_RELEASE(Texture, s_mirrorDepth);
        RAGE_RELEASE(Sampler, s_sampler);
        RAGE_RELEASE(GraphicsPipeline, s_pipe2d);
        RAGE_RELEASE(GraphicsPipeline, s_pipe2dSub);
        RAGE_RELEASE(Buffer, s_vertexBuffer);
        RAGE_RELEASE(TransferBuffer, s_vertexTransfer);
        RAGE_RELEASE(Texture, s_postTarget);
        RAGE_RELEASE(GraphicsPipeline, s_pipePost);
        RAGE_RELEASE(Sampler, s_samplerLinear);
        RAGE_RELEASE(Texture, s_finalTarget);
        RAGE_RELEASE(GraphicsPipeline, s_pipeComposite);
        for (slot = 0; slot < MODERN_RING; slot++)
            RAGE_RELEASE(Texture, s_ring[slot]);
#undef RAGE_RELEASE
    }
    free(s_vertices);
    free(s_spans);
    free(s_ringScene);
    ModernNativeGpuShutdown();
    s_vertices = NULL;
    s_spans = NULL;
    s_ringScene = NULL;
    s_resourcesReady = 0;
    s_haveRenderedFrame = 0;
    s_lastRenderedFrame = 0xFFFFFFFFu;
    s_vertexCount = s_spanCount = 0;
    s_ringNext = 0;
    if (hadResources && RageRuntimeConfigEnabled(
            "diagnostics.renderer_lifecycle", NULL)) {
        fprintf(stderr, "rage-port: modern resources destroyed generation=%u\n",
                s_resourceGeneration);
    }
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
    s_mirrorTargetW = (int)(148.0f * scale + 0.5f) & ~1;
    s_mirrorTargetH = (int)(36.0f * scale + 0.5f) & ~1;

    s_ringEnabled = RageRuntimeConfigEnabled(
        "diagnostics.marker_history", NULL);
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
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_mirrorTargetW;
        info.height = (Uint32)s_mirrorTargetH;
        s_mirrorTarget = SDL_CreateGPUTexture(s_device, &info);
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        s_mirrorDepth = SDL_CreateGPUTexture(s_device, &info);
    }
    {
        SDL_GPUSamplerCreateInfo info = {0};
        info.min_filter = SDL_GPU_FILTER_NEAREST;
        info.mag_filter = SDL_GPU_FILTER_NEAREST;
        s_sampler = SDL_CreateGPUSampler(s_device, &info);
    }
    vs = ModernCreateShader(
        modern_vert_spv, modern_vert_spv_len,
        MODERN_SHADER_MSL, MODERN_SHADER_MSL_SIZE,
        SDL_GPU_SHADERSTAGE_VERTEX, "vs_main", 0);
    fs = ModernCreateShader(
        modern_frag_spv, modern_frag_spv_len,
        MODERN_SHADER_MSL, MODERN_SHADER_MSL_SIZE,
        SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_main", 1);
    if (vs && fs) {
        s_pipe2d = ModernCreateOverlayPipeline(vs, fs, 0);
        s_pipe2dSub = ModernCreateOverlayPipeline(vs, fs, 1);
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
    if (!ModernNativeGpuInit(s_device)) {
        ModernDestroyResources();
        return 0;
    }

    if (s_ringEnabled) {
        SDL_GPUTextureCreateInfo info = {0};
        int slot;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_targetW;
        info.height = (Uint32)s_targetH;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        for (slot = 0; slot < MODERN_RING; slot++) {
            s_ring[slot] = SDL_CreateGPUTexture(s_device, &info);
        }
        s_ringScene = malloc(MODERN_RING * sizeof(RageSceneSnapshot));
    }

    if (s_config.modernPost != RAGE_MODERN_POST_NONE || s_config.modernGrading) {
        SDL_GPUSamplerCreateInfo samplerInfo = {0};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        s_samplerLinear = SDL_CreateGPUSampler(s_device, &samplerInfo);
    }
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
        if (!s_postTarget || !s_pipePost || !s_samplerLinear) {
            fprintf(stderr,
                    "rage-port: post-process setup failed, disabling: %s\n",
                    SDL_GetError());
            s_config.modernPost = RAGE_MODERN_POST_NONE;
        }
    }
    if (s_config.modernGrading) {
        SDL_GPUTextureCreateInfo info = {0};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_targetW;
        info.height = (Uint32)s_targetH;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_finalTarget = SDL_CreateGPUTexture(s_device, &info);
        s_pipeComposite = ModernCreateCompositePipeline();
        if (!s_finalTarget || !s_pipeComposite || !s_samplerLinear) {
            fprintf(stderr,
                    "rage-port: grading setup failed, disabling: %s\n",
                    SDL_GetError());
            s_config.modernGrading = 0;
        }
    }

    if (!s_target || !s_depth || !s_mirrorTarget || !s_mirrorDepth ||
        !s_sampler || !s_pipe2d || !s_pipe2dSub ||
        !s_vertexBuffer || !s_vertexTransfer || !s_vertices || !s_spans) {
        fprintf(stderr, "rage-port: modern renderer resource setup failed: %s\n",
                SDL_GetError());
        ModernDestroyResources();
        return 0;
    }
    s_resourcesReady = 1;
    s_resourceGeneration++;
    fprintf(stderr, "rage-port: modern renderer target %dx%d\n", s_targetW,
            s_targetH);
    if (RageRuntimeConfigEnabled("diagnostics.renderer_lifecycle", NULL))
        fprintf(stderr,
                "rage-port: modern resources created generation=%u size=%dx%d\n",
                s_resourceGeneration, s_targetW, s_targetH);
    return 1;
}

/* ---- frame building ---- */

typedef struct Modern2DState {
    uint32_t tpage;    /* from GP0(E1) for sprites */
    uint32_t twin;     /* current texture window bytes */
    SDL_Rect scissor;  /* current draw area, overlay pixels */
    int areaTopVram;   /* raw GP0(E3) row, VRAM-absolute */
    int hasScissor;
    int areaEmpty;
    int offsetX, offsetY; /* GP0(E5) relative offset */
} Modern2DState;

static uint8_t s_currentPass;
enum {
    MODERN_LAYER_HUD,
    MODERN_LAYER_MIRROR_FOREGROUND,
};
static uint8_t s_currentLayer;

static ModernSpan *ModernBeginSpan(int pipeline, const Modern2DState *state,
                                   float depthKey) {
    ModernSpan *span;
    if (s_spanCount > 0) {
        span = &s_spans[s_spanCount - 1];
        if (span->pipeline == pipeline && depthKey == span->depthKey &&
            span->pass == s_currentPass && span->layer == s_currentLayer &&
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
    span->layer = s_currentLayer;
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

/* ---- arbitrary-FPS presentation timing ---- */

static Uint64 s_tickTimeNs;
static Uint64 s_tickIntervalNs;
static uint32_t s_tickFrame = 0xFFFFFFFFu;
static Uint64 s_lastPresentationNs;

void RageModernLogicFrameReady(uint32_t frame) {
    Uint64 now = SDL_GetTicksNS();
    if (s_tickTimeNs != 0) {
        Uint64 delta = now - s_tickTimeNs;
        if (delta > 1000000 && delta < 200000000) s_tickIntervalNs = delta;
    }
    s_tickFrame = frame;
    s_tickTimeNs = now;
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

/* Drawing-area rows are VRAM-absolute; the PS1 draws a pixel only when it
 * lies inside the area AND the current page. Model that literally: keep the
 * raw E3 row and intersect [top..bottom] with the frame's page on E4. The
 * sliding mirror leans on this - a slide of zero rows arrives as
 * (top=pageY, bottom=pageY-1), and any folding heuristic that "repairs"
 * such rows turns a deliberately empty area into a full-height band (the
 * navy flash captured at ring frame 3103: (86,240)-(233,239) became
 * y=0,h=241). */
static int s_areaPageY;

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
        state->areaTopVram = y;
        state->hasScissor = 1;
        break;
    }
    case 0xE4: {
        int x = (int)(word & 0x3FFu);
        int y = (int)((word >> 10) & 0x1FFu);
        int top = state->areaTopVram > s_areaPageY ? state->areaTopVram
                                                   : s_areaPageY;
        int bottom = y < s_areaPageY + 239 ? y : s_areaPageY + 239;
        state->scissor.w = x - state->scissor.x + 1;
        state->scissor.y = top - s_areaPageY;
        state->scissor.h = bottom - top + 1;
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

/* ---- captured PS1 2D -> overlay vertex/span lists ---- */

static void ModernBuildOverlayFrame(const RageSceneSnapshot *snapshot) {
    Modern2DState state2d;
    int i;

    s_vertexCount = 0;
    s_spanCount = 0;
    s_areaPageY = snapshot->displayPageY;
    s_currentPass = 0;
    s_currentLayer = MODERN_LAYER_HUD;

    memset(&state2d, 0, sizeof(state2d));
    state2d.twin = 0x0000FFFFu;

    /* Native rendering owns the complete sky and all 3D. Retain only the
     * captured foreground UI while continuing to consume GPU state words. */
    for (i = 0; i < snapshot->packetCount; i++) {
        const RageCapturePacket *packet = &snapshot->packets[i];
        uint32_t command = packet->words[0] >> 24;
        if (packet->table != 0) continue;
        if (packet->bucket >= MODERN_BACKGROUND_BUCKET && command < 0xE0u) {
            continue;
        }
        ModernReplay2DPacket(packet, &state2d, MODERN_PIPE_2D);
    }

    /* The native mirror supplies its own world and backdrop. Retain only
     * captured 2D framing/text, seeded with the main table's final GPU state
     * so the original slide-in clipping still applies. */
    s_currentPass = 1;
    s_currentLayer = MODERN_LAYER_MIRROR_FOREGROUND;
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
    s_currentLayer = MODERN_LAYER_HUD;
}

/* ---- rendering ---- */

/* The texture the frame chain ends in: composite > fxaa > raw target. */
static SDL_GPUTexture *ModernPresentTexture(void) {
    if (s_config.modernGrading &&
        s_finalTarget != NULL && s_pipeComposite != NULL) {
        return s_finalTarget;
    }
    if (s_config.modernPost != RAGE_MODERN_POST_NONE && s_postTarget != NULL) {
        return s_postTarget;
    }
    return s_target;
}

static void ModernFullscreenPass(SDL_GPUCommandBuffer *cmd,
                                 SDL_GPUGraphicsPipeline *pipeline,
                                 SDL_GPUTexture *target,
                                 SDL_GPUTexture *sources[], int sourceCount) {
    const SDL_GPUColorTargetInfo color = {
        .texture = target,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &color, 1, NULL);
    SDL_GPUTextureSamplerBinding bindings[2];
    int i;
    if (pass == NULL) return;
    for (i = 0; i < sourceCount && i < 2; i++) {
        bindings[i].texture = sources[i];
        bindings[i].sampler = s_samplerLinear;
    }
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, (Uint32)sourceCount);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

static void ModernRenderOverlaySelection(SDL_GPUCommandBuffer *cmd,
                                         SDL_GPUTexture *vram, int passNumber,
                                         uint32_t layerMask, int clearColor) {
    SDL_GPUGraphicsPipeline *pipelines[2] = {s_pipe2d, s_pipe2dSub};
    SDL_GPUColorTargetInfo color = {
        .texture = s_target,
        .clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
        .load_op = clearColor ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPUDepthStencilTargetInfo depth = {
        .texture = s_depth,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &color, 1, &depth);
    SDL_GPUGraphicsPipeline *bound = NULL;
    SDL_GPUBufferBinding vertex = {.buffer = s_vertexBuffer, .offset = 0};
    SDL_GPUTextureSamplerBinding texture = {.texture = vram,
                                            .sampler = s_sampler};
    int spanIndex;
    if (pass == NULL) return;
    SDL_BindGPUVertexBuffers(pass, 0, &vertex, 1);
    SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
        const ModernSpan *span = &s_spans[spanIndex];
        SDL_Rect scissor = {0, 0, s_targetW, s_targetH};
        if (span->pass != passNumber || span->count == 0 ||
            (layerMask & (1u << span->layer)) == 0) continue;
        if (pipelines[span->pipeline] != bound) {
            bound = pipelines[span->pipeline];
            SDL_BindGPUGraphicsPipeline(pass, bound);
        }
        if (span->hasScissor) scissor = span->scissor;
        SDL_SetGPUScissor(pass, &scissor);
        SDL_DrawGPUPrimitives(pass, (Uint32)span->count, 1,
                              (Uint32)span->start, 0);
    }
    SDL_EndGPURenderPass(pass);
}

static void ModernCompositeNativeMirror(SDL_GPUCommandBuffer *cmd) {
    float panelY = ModernNativeGpuMirrorPanelY();
    float visibleTop = panelY < 0.0f ? 0.0f : panelY;
    float visibleBottom = panelY + 36.0f;
    float sourceTop;
    SDL_GPUBlitInfo blit = {0};

    if (!ModernNativeGpuHasMirrorDraws()) return;
    if (visibleBottom > 240.0f) visibleBottom = 240.0f;
    if (visibleBottom <= visibleTop) return;
    sourceTop = visibleTop - panelY;
    blit.source.texture = s_mirrorTarget;
    blit.source.y = (Uint32)lroundf(
        sourceTop * (float)s_mirrorTargetH / 36.0f);
    blit.source.w = (Uint32)s_mirrorTargetW;
    blit.source.h = (Uint32)lroundf(
        (visibleBottom - visibleTop) * (float)s_mirrorTargetH / 36.0f);
    blit.destination.texture = s_target;
    blit.destination.x = (Uint32)lroundf(
        (86.0f + s_overscanX) * (float)s_targetW / s_logicalW);
    blit.destination.y = (Uint32)lroundf(
        visibleTop * (float)s_targetH / 240.0f);
    blit.destination.w = (Uint32)lroundf(
        148.0f * (float)s_targetW / s_logicalW);
    blit.destination.h = (Uint32)lroundf(
        (visibleBottom - visibleTop) * (float)s_targetH / 240.0f);
    blit.load_op = SDL_GPU_LOADOP_LOAD;
    /* A physical rear-view mirror swaps left and right. Rendering from a
     * rear camera supplies the scene; flipping at composition supplies the
     * reflection instead of a turn-your-head view. */
    blit.flip_mode = SDL_FLIP_HORIZONTAL;
    blit.filter = s_config.modernTextureFilterLinear
                      ? SDL_GPU_FILTER_LINEAR
                      : SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(cmd, &blit);
}

static void ModernRender(const RageSceneSnapshot *snapshot) {
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUTexture *vram = Psyz_VideoGetVramTexture_SDL3GPU();
    static Uint64 profileBuildNs, profileSubmitNs;
    static Uint64 profileFaces, profileVertices, profileSpans;
    static unsigned profileFrames;
    static int profile = -1;
    Uint64 profileStart = 0, profileBuilt = 0;
    int i;
    if (vram == NULL) return;
    if (profile < 0)
        profile = RageRuntimeConfigEnabled("diagnostics.performance", NULL);
    if (profile) profileStart = SDL_GetTicksNS();
    ModernBuildOverlayFrame(snapshot);
    if (profile) profileBuilt = SDL_GetTicksNS();
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
        static uint64_t reportedIncompleteFrame = UINT64_MAX;
        ModernNativeGpuDraw(cmd, s_target, s_depth, 1);
        if (!ModernNativeGpuWorldComplete() &&
            reportedIncompleteFrame != snapshot->frameCounter) {
            reportedIncompleteFrame = snapshot->frameCounter;
            fprintf(stderr,
                    "rage-port: incomplete native world at frame %u; "
                    "legacy 3D fallback is disabled\n",
                    snapshot->frameCounter);
        }
        ModernRenderOverlaySelection(cmd, vram, 0,
                                     1u << MODERN_LAYER_HUD, 0);
        if (ModernNativeGpuHasMirrorDraws()) {
            ModernNativeGpuDrawMirror(cmd, s_mirrorTarget, s_mirrorDepth);
            ModernCompositeNativeMirror(cmd);
        }
        ModernRenderOverlaySelection(cmd, vram, 1,
                                     1u << MODERN_LAYER_MIRROR_FOREGROUND, 0);
    }
    {
        SDL_GPUTexture *chain = s_target;
        if (s_config.modernPost != RAGE_MODERN_POST_NONE &&
            s_pipePost != NULL && s_postTarget != NULL) {
            SDL_GPUTexture *sources[1];
            sources[0] = chain;
            ModernFullscreenPass(cmd, s_pipePost, s_postTarget, sources, 1);
            chain = s_postTarget;
        }
        if (s_config.modernGrading &&
            s_pipeComposite != NULL && s_finalTarget != NULL) {
            SDL_GPUTexture *sources[1];
            sources[0] = chain;
            ModernFullscreenPass(cmd, s_pipeComposite, s_finalTarget, sources, 1);
        }
    }
    if (s_ringEnabled && s_ring[s_ringNext] != NULL) {
        const SDL_GPUBlitInfo blit = {
            .source = {.texture = ModernPresentTexture(),
                       .w = (Uint32)s_targetW,
                       .h = (Uint32)s_targetH},
            .destination = {.texture = s_ring[s_ringNext],
                            .w = (Uint32)s_targetW,
                            .h = (Uint32)s_targetH},
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .filter = SDL_GPU_FILTER_NEAREST,
        };
        SDL_BlitGPUTexture(cmd, &blit);
        s_ringFrame[s_ringNext] = snapshot->frameCounter;
        s_ringT[s_ringNext] = -1.0f;
        if (s_ringScene != NULL) {
            memcpy(&s_ringScene[s_ringNext], snapshot, sizeof(*snapshot));
        }
        s_ringNext = (s_ringNext + 1) % MODERN_RING;
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    if (profile) {
        Uint64 finished = SDL_GetTicksNS();
        profileBuildNs += profileBuilt - profileStart;
        profileSubmitNs += finished - profileBuilt;
        profileFaces += (Uint64)snapshot->faceCount;
        profileVertices += (Uint64)s_vertexCount;
        profileSpans += (Uint64)s_spanCount;
        profileFrames++;
        if (profileFrames == 120) {
            fprintf(stderr,
                    "modern-profile frames=%u build_ms=%.3f submit_ms=%.3f "
                    "faces=%.0f vertices=%.0f spans=%.0f\n",
                    profileFrames,
                    (double)profileBuildNs / profileFrames / 1000000.0,
                    (double)profileSubmitNs / profileFrames / 1000000.0,
                    (double)profileFaces / profileFrames,
                    (double)profileVertices / profileFrames,
                    (double)profileSpans / profileFrames);
            profileBuildNs = profileSubmitNs = 0;
            profileFaces = profileVertices = profileSpans = 0;
            profileFrames = 0;
        }
    }
    s_haveRenderedFrame = 1;
    if (RageRuntimeConfigEnabled("diagnostics.modern_span_trace", "RAGE_PORT_MODERN_SPAN_TRACE")) {
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

static RageModernDiagnosticFrame ModernDiagnosticFrame(void) {
    RageModernDiagnosticFrame frame = {
        .device = s_device,
        .texture = ModernPresentTexture(),
        .width = s_targetW,
        .height = s_targetH,
        .logicalWidth = s_logicalW,
        .fps = s_config.modernFps,
        .ringTextures = s_ring,
        .ringFrames = s_ringFrame,
        .ringInterpolation = s_ringT,
        .ringScenes = s_ringScene,
        .ringCount = s_ringEnabled ? MODERN_RING : 0,
        .ringNext = s_ringNext,
    };
    return frame;
}

/* Diagnostic: write the modern target as a binary PPM when
 * RAGE_PORT_MODERN_DUMP names a path and RAGE_PORT_MODERN_DUMP_FRAME (if
 * set) matches the captured frame counter. */
static void ModernMaybeDump(const RageSceneSnapshot *snapshot) {
    RageModernDiagnosticFrame frame = ModernDiagnosticFrame();
    RageModernDiagnosticsMaybeDump(snapshot, &frame);
}

/* Debug marker: pressing M writes markers/marker-N-{modern,compat}.ppm,
 * an info.txt with camera/draw/terrain state and the raw scene snapshot,
 * so a moment the player flags can be analysed offline. Works in menus and
 * attract too (compat image only when the scene has no 3D). */
static void ModernMarkerCheck(const RageSceneSnapshot *snapshot,
                              int haveModernImage) {
    RageModernDiagnosticFrame frame = ModernDiagnosticFrame();
    RageModernDiagnosticsCheckMarker(snapshot, &frame, haveModernImage);
}

/* ---- hooks ---- */

static void ModernOverlayInit(SDL_Window *window, SDL_GPUDevice *device) {
    if (s_device != NULL && s_device != device) {
        ModernDestroyResources();
    }
    s_window = window;
    s_device = device;
    if (s_prev_overlay_init) {
        s_prev_overlay_init(window, device);
    }
}

static void ModernPresentSource(PsyzPresentSourceInfo *info) {
    const RageSceneSnapshot *snapshot;
    int fpsMode;
    const bool *keys;
    int toggleDown;
    if (s_prev_present_source) {
        s_prev_present_source(info);
    }
    keys = SDL_GetKeyboardState(NULL);
    toggleDown = keys != NULL && keys[s_toggleScancode];
    if (toggleDown && !s_toggleWasDown) RageModernToggle();
    s_toggleWasDown = toggleDown;
    {
        /* M writes what the modern renderer is showing, with the state that
         * produced it, so a player who can see something wrong can hand over
         * the picture and the place it happened. */
        extern int g_SceneId;
        extern s32 g_SceneTimer;
        static int markWasDown, marks;
        int markDown = keys != NULL && keys[SDL_SCANCODE_M];
        if (markDown && !markWasDown) {
            char directory[1024], path[1200];
            if (!RagePlatformUserConfigDirectory(directory, sizeof(directory)))
                snprintf(directory, sizeof(directory), ".");
            snprintf(path, sizeof(path), "%s/mark-%02d.ppm", directory, ++marks);
            fprintf(stderr,
                    "rage-port: mark %d scene=%d timer=%d course=%d mirror=%d "
                    "point=%d pos=%d,%d heading=%d camera=%d,%d,%d angle=%d view=%d\n",
                    marks, g_SceneId, g_SceneTimer, g_CourseIndex, g_MirrorMode,
                    g_PlayerCar.trackPointIndex, g_PlayerCar.x, g_PlayerCar.z,
                    g_PlayerCar.headingAngle, SCRATCH_VIEW_X, SCRATCH_VIEW_Y,
                    SCRATCH_VIEW_Z, SCRATCH_VIEW_ANGLE_Y, g_CameraViewMode);
            if (RageModernCaptureFrame(path))
                fprintf(stderr, "rage-port: mark %d written to %s\n", marks, path);
            else
                fprintf(stderr, "rage-port: mark %d could not be written\n", marks);
        }
        markWasDown = markDown;
    }
    if (!s_enabled || s_device == NULL) return;
    fpsMode = s_config.modernFps != RAGE_MODERN_FPS_LOGIC;
    /* Both modes render the PREVIOUS logic frame - the one compat is
     * presenting during this tick; fps mode moves its transforms toward
     * the newest frame by the wall-clock fraction of the tick. */
    snapshot = RageCapturePrevious();
    {
        int passthrough =
            snapshot->faceCount == 0 ||
            (snapshot->displayHeight != 0 && snapshot->displayHeight != 240);
        ModernMarkerCheck(snapshot, !passthrough && s_resourcesReady &&
                                        s_haveRenderedFrame);
    }
    /* Scenes with no captured 3D pass through to the compat image, and so
     * do 480-line menu scenes: their double-height buffer follows PS1
     * interlace conventions the compat presenter already handles. */
    if (snapshot->faceCount == 0) {
        /* Menus and other 2D-only scenes use the compatibility framebuffer,
         * but presentation should still honour the modern texture-filter
         * setting. Filtering the completed framebuffer cannot bleed between
         * atlas entries and avoids exposing native texel stair-steps as
         * apparent splits through large glyphs. */
        if (s_config.modernTextureFilterLinear) {
            info->filter = SDL_GPU_FILTER_LINEAR;
        }
        return;
    }
    if (snapshot->displayHeight != 0 && snapshot->displayHeight != 240) {
        return;
    }
    if (!ModernEnsureResources()) return;
    if (fpsMode) {
        const RageSceneSnapshot *target = RageCaptureCurrent();
        Uint64 now = SDL_GetTicksNS();
        float t = 1.0f;
        /* RagePortAfterSceneHandler timestamps the completed logic frame.
         * Starting interpolation when it is first presented instead made
         * the first repeated frame consume part of the next tick. */
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
        ModernNativeGpuPrepare(RageGameRenderWorldPresentation(t),
                               (float)s_targetW / (float)s_targetH);
        ModernRender(snapshot);
        s_lastPresentationNs = now;
        if (s_haveRenderedFrame) ModernMaybeDump(snapshot);
    } else if (snapshot->frameCounter != s_lastRenderedFrame) {
        const RageRenderWorld *world = RageGameRenderWorldPrevious();
        if (world == NULL) world = RageGameRenderWorldCurrent();
        ModernNativeGpuPrepare(world, (float)s_targetW / (float)s_targetH);
        ModernRender(snapshot);
        s_lastRenderedFrame = snapshot->frameCounter;
        if (s_haveRenderedFrame) ModernMaybeDump(snapshot);
    }
    if (!s_haveRenderedFrame) return;
    info->texture = ModernPresentTexture();
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
        /* Include the normal end-of-tick present in pacing. Tracking only
         * intermediate presents emitted one immediately after the next
         * logic update, producing an 8/30 ms cadence that still looked like
         * the original 25-30 FPS despite interpolation. */
        if (now - s_lastPresentationNs < interval) return;
    }
    Psyz_VideoPresentIntermediate();
}

int RageModernInit(const RagePortConfig *config) {
    const char *toggleKey;
    if (s_initialized) {
        return 1;
    }
    s_config = *config;
    if (config->renderer == RAGE_RENDERER_MODERN && !ModernAssetsInit()) {
        fprintf(stderr,
                "rage-port: refusing to start modern renderer without "
                "native assets\n");
        return 0;
    }
    toggleKey = RageRuntimeConfigGet("video.toggle_renderer_key");
    if (toggleKey != NULL && toggleKey[0] != '\0') {
        SDL_Scancode parsed = SDL_GetScancodeFromName(toggleKey);
        if (parsed != SDL_SCANCODE_UNKNOWN) s_toggleScancode = parsed;
        else fprintf(stderr, "rage-port: unknown renderer toggle key %s; using F10\n",
                     toggleKey);
    }
    s_prev_overlay_init = Psyz_OverlayInit_SDL3GPU(ModernOverlayInit);
    s_prev_present_source = Psyz_PresentSource_SDL3GPU(ModernPresentSource);
    s_initialized = 1;
    s_tickTimeNs = s_tickIntervalNs = s_lastPresentationNs = 0;
    s_tickFrame = 0xFFFFFFFFu;
    s_enabled = config->renderer == RAGE_RENDERER_MODERN;
    fprintf(stderr, "rage-port: renderer toggle=%s; active=%s\n",
            SDL_GetScancodeName(s_toggleScancode),
            s_enabled ? "modern" : "classic");
    return 1;
}

void RageModernShutdown(void) {
    if (!s_initialized) {
        return;
    }
    Psyz_PresentSource_SDL3GPU(s_prev_present_source);
    Psyz_OverlayInit_SDL3GPU(s_prev_overlay_init);
    ModernDestroyResources();
    ModernAssetsShutdown();
    s_prev_present_source = NULL;
    s_prev_overlay_init = NULL;
    s_window = NULL;
    s_device = NULL;
    s_enabled = 0;
    s_initialized = 0;
}

int RageModernIsEnabled(void) {
    return s_enabled;
}

void RageModernToggle(void) {
    if (!s_initialized) return;
    if (!s_enabled && !ModernAssetsReady() && !ModernAssetsInit()) {
        fprintf(stderr,
                "rage-port: renderer switch to modern refused: native "
                "assets are unavailable\n");
        return;
    }
    s_enabled = !s_enabled;
    if (!s_enabled) {
        ModernDestroyResources();
    }
    s_lastRenderedFrame = 0xFFFFFFFFu;
    fprintf(stderr, "rage-port: renderer switched to %s\n",
            s_enabled ? "modern" : "classic");
}

/* Read back what the modern renderer is presenting. The frame capture the
 * smoke executable uses reads PS1 video memory, which the modern renderer
 * never writes to: it presents a texture of its own. Without this its output
 * cannot be looked at, which is the only way to tell whether the geometry it
 * draws is right. */
int RageModernCaptureFrame(const char *path) {
    if (!s_enabled || s_device == NULL || path == NULL || path[0] == '\0')
        return 0;
    return RageModernWriteTexturePpm(s_device, ModernPresentTexture(),
                                     s_targetW, s_targetH, path);
}

int RageModernCullMarginX(void) {
    if (!s_enabled || s_config.modernAspect != RAGE_MODERN_ASPECT_16_9) {
        return 0;
    }
    /* Half the widened logical width, rounded up: 320*(16/9)/(4/3) adds
     * ~53.3 columns per side. */
    return 54;
}

int RageModernDepthLimit(void) {
    float multiplier;
    if (!s_enabled) return 0;
    multiplier = s_config.modernDrawDistance;
    if (multiplier <= 1.0f) return 0;
    if (multiplier > (float)MODERN_FAR / 448.0f)
        multiplier = (float)MODERN_FAR / 448.0f;
    return (int)(448.0f * multiplier);
}
