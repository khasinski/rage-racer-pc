#include "modern_gpu_resources.h"
#include "modern_frame_types.h"
#include "modern_shader_sources.h"
#include "shaders/modern_vert_spv.h"
#include "shaders/modern_frag_spv.h"
#include "shaders/post_vert_spv.h"
#include "shaders/post_frag_spv.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_GPUShader *CreateShader(SDL_GPUDevice *device,
                                   const unsigned char *spirv,
                                   size_t spirvSize, const char *msl,
                                   size_t mslSize, SDL_GPUShaderStage stage,
                                   const char *mslEntry, int samplers) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
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
    return SDL_CreateGPUShader(device, &info);
}

static SDL_GPUGraphicsPipeline *CreateFullscreenPipeline(
    SDL_GPUDevice *device, const unsigned char *vsSpirv, size_t vsSpirvSize,
    const char *vsMsl, size_t vsMslSize, const char *vsMslEntry,
    const unsigned char *fsSpirv, size_t fsSpirvSize, const char *fsMsl,
    size_t fsMslSize, const char *fsMslEntry, int samplers) {
    SDL_GPUShader *vs = CreateShader(device, vsSpirv, vsSpirvSize, vsMsl,
                                     vsMslSize, SDL_GPU_SHADERSTAGE_VERTEX,
                                     vsMslEntry, 0);
    SDL_GPUShader *fs = CreateShader(device, fsSpirv, fsSpirvSize, fsMsl,
                                     fsMslSize, SDL_GPU_SHADERSTAGE_FRAGMENT,
                                     fsMslEntry, samplers);
    SDL_GPUGraphicsPipeline *pipeline = NULL;
    if (vs && fs) {
        const SDL_GPUColorTargetDescription target = {
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
        SDL_GPUGraphicsPipelineCreateInfo info = {0};
        info.vertex_shader = vs;
        info.fragment_shader = fs;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    }
    if (vs) SDL_ReleaseGPUShader(device, vs);
    if (fs) SDL_ReleaseGPUShader(device, fs);
    return pipeline;
}

static SDL_GPUGraphicsPipeline *CreateGeometryPipeline(
    SDL_GPUDevice *device, SDL_GPUShader *vs, SDL_GPUShader *fs,
    int depthTest, int depthWrite, int subtract) {
    const SDL_GPUVertexBufferDescription buffer = {
        .slot = 0, .pitch = sizeof(ModernVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX};
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
         .offset = offsetof(ModernVertex, clut)}};
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
            .enable_blend = true}};
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
        depthTest ? SDL_GPU_COMPAREOP_LESS_OR_EQUAL : SDL_GPU_COMPAREOP_ALWAYS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = depthWrite != 0;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(device, &info);
}

static SDL_GPUGraphicsPipeline *CreateCompositePipeline(
    SDL_GPUDevice *device, const RagePortConfig *config) {
    char header[128];
    size_t headerLen;
    char *source;
    size_t sourceSize;
    SDL_GPUGraphicsPipeline *pipeline;
    snprintf(header, sizeof(header),
             "constant float kBloom = %.4f;\nconstant int kGrading = %d;\n",
             (double)config->modernBloom, config->modernGrading);
    headerLen = strlen(header);
    sourceSize = MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1 + headerLen +
                 MODERN_COMPOSITE_MSL_SIZE;
    source = malloc(sourceSize);
    if (source == NULL) return NULL;
    memcpy(source, MODERN_COMPOSITE_PROLOGUE_MSL,
           MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1);
    memcpy(source + MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1, header, headerLen);
    memcpy(source + MODERN_COMPOSITE_PROLOGUE_MSL_SIZE - 1 + headerLen,
           MODERN_COMPOSITE_MSL, MODERN_COMPOSITE_MSL_SIZE);
    pipeline = CreateFullscreenPipeline(
        device, NULL, 0, MODERN_EFFECTS_MSL, MODERN_EFFECTS_MSL_SIZE, "vs_fx",
        NULL, 0, source, sourceSize, "fs_composite", 2);
    free(source);
    return pipeline;
}

void ModernGpuResourcesInit(ModernGpuResources *resources) {
    memset(resources, 0, sizeof(*resources));
}

int ModernGpuResourcesCreate(ModernGpuResources *resources,
                             SDL_GPUDevice *device, RagePortConfig *config,
                             float *logicalWidth, float *overscanX,
                             int ringEnabled) {
    SDL_GPUShader *vs;
    SDL_GPUShader *fs;
    SDL_GPUTextureCreateInfo texture = {0};
    float scale = config->modernInternalScale;
    int slot;

    if (resources->ready) return 1;
    if (device == NULL) return 0;
    if (scale < 0.5f) scale = 0.5f;
    *logicalWidth = config->modernAspect == RAGE_MODERN_ASPECT_16_9
                        ? 320.0f * (16.0f / 9.0f) / (4.0f / 3.0f)
                        : 320.0f;
    *overscanX = (*logicalWidth - 320.0f) * 0.5f;
    resources->targetW = (int)(*logicalWidth * scale + 0.5f) & ~1;
    resources->targetH = (int)(240.0f * scale + 0.5f) & ~1;

    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                    SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture.width = (Uint32)resources->targetW;
    texture.height = (Uint32)resources->targetH;
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;
    resources->target = SDL_CreateGPUTexture(device, &texture);
    texture.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    texture.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    resources->depth = SDL_CreateGPUTexture(device, &texture);
    {
        SDL_GPUSamplerCreateInfo sampler = {0};
        sampler.min_filter = SDL_GPU_FILTER_NEAREST;
        sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
        resources->sampler = SDL_CreateGPUSampler(device, &sampler);
    }

    vs = CreateShader(device, modern_vert_spv, modern_vert_spv_len,
                      MODERN_SHADER_MSL, MODERN_SHADER_MSL_SIZE,
                      SDL_GPU_SHADERSTAGE_VERTEX, "vs_main", 0);
    fs = CreateShader(device, modern_frag_spv, modern_frag_spv_len,
                      MODERN_SHADER_MSL, MODERN_SHADER_MSL_SIZE,
                      SDL_GPU_SHADERSTAGE_FRAGMENT, "fs_main", 1);
    if (vs && fs) {
        resources->pipe3dOpaque =
            CreateGeometryPipeline(device, vs, fs, 1, 1, 0);
        resources->pipe3dBlend =
            CreateGeometryPipeline(device, vs, fs, 1, 0, 0);
        resources->pipe3dSub =
            CreateGeometryPipeline(device, vs, fs, 1, 0, 1);
        resources->pipe2d = CreateGeometryPipeline(device, vs, fs, 0, 0, 0);
        resources->pipe2dSub =
            CreateGeometryPipeline(device, vs, fs, 0, 0, 1);
    }
    if (vs) SDL_ReleaseGPUShader(device, vs);
    if (fs) SDL_ReleaseGPUShader(device, fs);
    {
        SDL_GPUBufferCreateInfo buffer = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = MODERN_MAX_VERTICES * sizeof(ModernVertex)};
        resources->vertexBuffer = SDL_CreateGPUBuffer(device, &buffer);
    }
    {
        SDL_GPUTransferBufferCreateInfo transfer = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = MODERN_MAX_VERTICES * sizeof(ModernVertex)};
        resources->vertexTransfer =
            SDL_CreateGPUTransferBuffer(device, &transfer);
    }

    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                    SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if (ringEnabled) {
        for (slot = 0; slot < MODERN_GPU_RING_SIZE; slot++)
            resources->ring[slot] = SDL_CreateGPUTexture(device, &texture);
    }
    if (config->modernPost != RAGE_MODERN_POST_NONE ||
        config->modernBloom > 0.0f || config->modernGrading) {
        SDL_GPUSamplerCreateInfo sampler = {0};
        sampler.min_filter = SDL_GPU_FILTER_LINEAR;
        sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
        resources->samplerLinear = SDL_CreateGPUSampler(device, &sampler);
    }
    if (config->modernPost != RAGE_MODERN_POST_NONE) {
        resources->postTarget = SDL_CreateGPUTexture(device, &texture);
        resources->pipePost = CreateFullscreenPipeline(
            device, post_vert_spv, post_vert_spv_len, MODERN_POST_MSL,
            MODERN_POST_MSL_SIZE, "vs_post", post_frag_spv,
            post_frag_spv_len, MODERN_POST_MSL, MODERN_POST_MSL_SIZE,
            "fs_post", 1);
        if (!resources->postTarget || !resources->pipePost ||
            !resources->samplerLinear) {
            fprintf(stderr,
                    "rage-port: post-process setup failed, disabling: %s\n",
                    SDL_GetError());
            config->modernPost = RAGE_MODERN_POST_NONE;
        }
    }
    if (config->modernBloom > 0.0f || config->modernGrading) {
        resources->finalTarget = SDL_CreateGPUTexture(device, &texture);
        resources->pipeComposite = CreateCompositePipeline(device, config);
        if (config->modernBloom > 0.0f) {
            resources->bloomW = resources->targetW / 4 > 0
                                    ? resources->targetW / 4 : 1;
            resources->bloomH = resources->targetH / 4 > 0
                                    ? resources->targetH / 4 : 1;
            texture.width = (Uint32)resources->bloomW;
            texture.height = (Uint32)resources->bloomH;
            resources->bloomA = SDL_CreateGPUTexture(device, &texture);
            resources->bloomB = SDL_CreateGPUTexture(device, &texture);
            resources->pipeBright = CreateFullscreenPipeline(
                device, NULL, 0, MODERN_EFFECTS_MSL,
                MODERN_EFFECTS_MSL_SIZE, "vs_fx", NULL, 0,
                MODERN_EFFECTS_MSL, MODERN_EFFECTS_MSL_SIZE, "fs_bright", 1);
            resources->pipeBlurH = CreateFullscreenPipeline(
                device, NULL, 0, MODERN_EFFECTS_MSL,
                MODERN_EFFECTS_MSL_SIZE, "vs_fx", NULL, 0,
                MODERN_EFFECTS_MSL, MODERN_EFFECTS_MSL_SIZE, "fs_blur_h", 1);
            resources->pipeBlurV = CreateFullscreenPipeline(
                device, NULL, 0, MODERN_EFFECTS_MSL,
                MODERN_EFFECTS_MSL_SIZE, "vs_fx", NULL, 0,
                MODERN_EFFECTS_MSL, MODERN_EFFECTS_MSL_SIZE, "fs_blur_v", 1);
        }
        if (!resources->finalTarget || !resources->pipeComposite ||
            !resources->samplerLinear ||
            (config->modernBloom > 0.0f &&
             (!resources->bloomA || !resources->bloomB ||
              !resources->pipeBright || !resources->pipeBlurH ||
              !resources->pipeBlurV))) {
            fprintf(stderr,
                    "rage-port: bloom/grading setup failed, disabling: %s\n",
                    SDL_GetError());
            config->modernBloom = 0.0f;
            config->modernGrading = 0;
        }
    }

    if (!resources->target || !resources->depth || !resources->sampler ||
        !resources->pipe3dOpaque || !resources->pipe3dBlend ||
        !resources->pipe3dSub || !resources->pipe2d || !resources->pipe2dSub ||
        !resources->vertexBuffer || !resources->vertexTransfer) {
        fprintf(stderr, "rage-port: modern renderer resource setup failed: %s\n",
                SDL_GetError());
        ModernGpuResourcesRelease(resources, device);
        return 0;
    }
    resources->ready = 1;
    resources->generation++;
    return 1;
}

void ModernGpuResourcesRelease(ModernGpuResources *resources,
                               SDL_GPUDevice *device) {
    unsigned int generation = resources->generation;
    int slot;

    if (device != NULL) {
#define RAGE_RELEASE(kind, value) do {                                        \
        if ((value) != NULL) SDL_ReleaseGPU##kind(device, (value));           \
    } while (0)
        RAGE_RELEASE(Texture, resources->target);
        RAGE_RELEASE(Texture, resources->depth);
        RAGE_RELEASE(Sampler, resources->sampler);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dOpaque);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dBlend);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dSub);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe2d);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe2dSub);
        RAGE_RELEASE(Buffer, resources->vertexBuffer);
        RAGE_RELEASE(TransferBuffer, resources->vertexTransfer);
        RAGE_RELEASE(Texture, resources->postTarget);
        RAGE_RELEASE(GraphicsPipeline, resources->pipePost);
        RAGE_RELEASE(Sampler, resources->samplerLinear);
        RAGE_RELEASE(Texture, resources->finalTarget);
        RAGE_RELEASE(Texture, resources->bloomA);
        RAGE_RELEASE(Texture, resources->bloomB);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBright);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBlurH);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBlurV);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeComposite);
        for (slot = 0; slot < MODERN_GPU_RING_SIZE; slot++)
            RAGE_RELEASE(Texture, resources->ring[slot]);
#undef RAGE_RELEASE
    }
    memset(resources, 0, sizeof(*resources));
    resources->generation = generation;
}
