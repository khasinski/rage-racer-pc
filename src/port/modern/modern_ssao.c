#include "modern_ssao.h"

#include <stdio.h>
#include <string.h>

#include "shaders/ssao_blur_frag_msl.h"
#include "shaders/ssao_blur_frag_spv.h"
#include "shaders/ssao_frag_msl.h"
#include "shaders/ssao_frag_spv.h"

/* World units. The first shadow cascade is a thousand units across; a car's
 * wheel arch is a few tens. */
#define SSAO_RADIUS 90.0f
#define SSAO_STRENGTH 2.2f
#define SSAO_FADE_START 3000.0f
#define SSAO_FADE_END 12000.0f

typedef struct SsaoUniform {
    float projection[4];
    float sampling[4];
    float screen[4];
} SsaoUniform;
/* Three std140 vec4s, as the shaders declare them. */
_Static_assert(sizeof(SsaoUniform) == 3 * 4 * sizeof(float),
               "SsaoUniform must match the Ssao uniform block");

static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_occlusionPipeline;
static SDL_GPUGraphicsPipeline *s_blurPipeline;
static SDL_GPUGraphicsPipeline *s_applyPipeline;
static SDL_GPUSampler *s_sampler;
static SDL_GPUTexture *s_raw;
static SDL_GPUTexture *s_blurred;
static int s_width, s_height;

static SDL_GPUShader *CreateShader(SDL_GPUShaderStage stage,
                                   const unsigned char *spirv, size_t spirvSize,
                                   const unsigned char *msl, size_t mslSize,
                                   const char *mslEntry, Uint32 samplers,
                                   Uint32 uniforms) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(s_device);
    SDL_GPUShaderCreateInfo info;

    SDL_zero(info);
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = spirv;
        info.code_size = spirvSize;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.code = msl;
        info.code_size = mslSize;
        info.entrypoint = mslEntry;
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(s_device, &info);
}

static SDL_GPUGraphicsPipeline *CreatePipeline(
    SDL_GPUShader *vertex, SDL_GPUShader *fragment, SDL_GPUTextureFormat format,
    int multiply) {
    SDL_GPUColorTargetDescription target;
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUGraphicsPipeline *pipeline = NULL;

    if (vertex == NULL || fragment == NULL) return NULL;
    SDL_zero(target);
    target.format = format;
    if (multiply) {
        /* result = destination * source: occlusion darkens what is there. */
        target.blend_state.enable_blend = true;
        target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_COLOR;
        target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    }
    SDL_zero(info);
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;
    pipeline = SDL_CreateGPUGraphicsPipeline(s_device, &info);
    return pipeline;
}

static SDL_GPUTexture *CreateOcclusionTexture(void) {
    SDL_GPUTextureCreateInfo info;

    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = (Uint32)s_width;
    info.height = (Uint32)s_height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(s_device, &info);
}

void ModernSsaoDestroy(void) {
    if (s_device != NULL) {
        if (s_raw != NULL) SDL_ReleaseGPUTexture(s_device, s_raw);
        if (s_blurred != NULL) SDL_ReleaseGPUTexture(s_device, s_blurred);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_occlusionPipeline != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_occlusionPipeline);
        if (s_blurPipeline != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_blurPipeline);
        if (s_applyPipeline != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_applyPipeline);
    }
    s_raw = s_blurred = NULL;
    s_sampler = NULL;
    s_occlusionPipeline = s_blurPipeline = s_applyPipeline = NULL;
    s_device = NULL;
    s_width = s_height = 0;
}

int ModernSsaoReady(void) { return s_applyPipeline != NULL; }

int ModernSsaoCreate(SDL_GPUDevice *device, SDL_GPUShader *fullscreenVertex,
                    int width, int height) {
    SDL_GPUShader *occlusion, *blur;
    SDL_GPUSamplerCreateInfo sampler;

    ModernSsaoDestroy();
    if (device == NULL || width <= 0 || height <= 0) return 0;
    s_device = device;
    s_width = width;
    s_height = height;
    occlusion = CreateShader(SDL_GPU_SHADERSTAGE_FRAGMENT, ssao_frag_spv,
                             ssao_frag_spv_len, ssao_frag_msl, ssao_frag_msl_len,
                             "fs_ssao", 1, 1);
    blur = CreateShader(SDL_GPU_SHADERSTAGE_FRAGMENT, ssao_blur_frag_spv,
                        ssao_blur_frag_spv_len, ssao_blur_frag_msl,
                        ssao_blur_frag_msl_len, "fs_ssao_blur", 2, 1);
    s_occlusionPipeline = CreatePipeline(fullscreenVertex, occlusion, SDL_GPU_TEXTUREFORMAT_R8_UNORM, 0);
    s_blurPipeline = CreatePipeline(fullscreenVertex, blur, SDL_GPU_TEXTUREFORMAT_R8_UNORM, 0);
    s_applyPipeline = CreatePipeline(fullscreenVertex, blur, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, 1);
    if (occlusion != NULL) SDL_ReleaseGPUShader(device, occlusion);
    if (blur != NULL) SDL_ReleaseGPUShader(device, blur);
    SDL_zero(sampler);
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s_sampler = SDL_CreateGPUSampler(device, &sampler);
    s_raw = CreateOcclusionTexture();
    s_blurred = CreateOcclusionTexture();
    if (s_occlusionPipeline == NULL || s_blurPipeline == NULL ||
        s_applyPipeline == NULL || s_sampler == NULL || s_raw == NULL ||
        s_blurred == NULL) {
        fprintf(stderr, "rage-port: ambient occlusion setup failed: %s\n",
                SDL_GetError());
        ModernSsaoDestroy();
        return 0;
    }
    return 1;
}

static void FullscreenPass(SDL_GPUCommandBuffer *command,
                           SDL_GPUGraphicsPipeline *pipeline,
                           SDL_GPUTexture *target, int loadTarget,
                           SDL_GPUTexture *first, SDL_GPUTexture *second,
                           const SsaoUniform *uniform) {
    SDL_GPUColorTargetInfo color;
    SDL_GPURenderPass *pass;
    SDL_GPUTextureSamplerBinding bindings[2];
    Uint32 count = 0;

    SDL_zero(color);
    color.texture = target;
    color.load_op = loadTarget ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_DONT_CARE;
    color.store_op = SDL_GPU_STOREOP_STORE;
    pass = SDL_BeginGPURenderPass(command, &color, 1, NULL);
    if (pass == NULL) return;
    bindings[count].texture = first;
    bindings[count++].sampler = s_sampler;
    if (second != NULL) {
        bindings[count].texture = second;
        bindings[count++].sampler = s_sampler;
    }
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, count);
    SDL_PushGPUFragmentUniformData(command, 0, uniform, sizeof(*uniform));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void ModernSsaoApply(SDL_GPUCommandBuffer *command, SDL_GPUTexture *sceneColor,
                     SDL_GPUTexture *sceneDepth, const ModernSsaoView *view) {
    SsaoUniform uniform;
    float texelX, texelY;

    if (!ModernSsaoReady() || command == NULL || sceneColor == NULL ||
        sceneDepth == NULL || view == NULL) {
        return;
    }
    texelX = 1.0f / (float)s_width;
    texelY = 1.0f / (float)s_height;
    memcpy(uniform.projection, view->projection, sizeof(uniform.projection));
    uniform.sampling[0] = SSAO_RADIUS;
    uniform.sampling[1] = SSAO_STRENGTH;
    uniform.sampling[2] = SSAO_FADE_START;
    uniform.sampling[3] = SSAO_FADE_END;
    uniform.screen[0] = texelX;
    uniform.screen[1] = texelY;
    uniform.screen[2] = 0.0f;
    uniform.screen[3] = 0.0f;
    FullscreenPass(command, s_occlusionPipeline, s_raw, 0, sceneDepth, NULL,
                   &uniform);
    uniform.screen[2] = texelX;
    FullscreenPass(command, s_blurPipeline, s_blurred, 0, s_raw, sceneDepth,
                   &uniform);
    uniform.screen[2] = 0.0f;
    uniform.screen[3] = texelY;
    FullscreenPass(command, s_applyPipeline, sceneColor, 1, s_blurred,
                   sceneDepth, &uniform);
}
