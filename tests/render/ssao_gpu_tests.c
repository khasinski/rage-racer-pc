/* Ambient occlusion on a real device: the effect comes and goes cleanly, and
 * it darkens a surface beside a nearer one while leaving a flat one alone. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "modern_ssao.h"
#include "post_vert_msl.h"
#include "post_vert_spv.h"

#define SIZE 128
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

static SDL_GPUShader *FullscreenVertex(SDL_GPUDevice *device) {
    SDL_GPUShaderCreateInfo info;
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);

    SDL_zero(info);
    info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = post_vert_spv;
        info.code_size = post_vert_spv_len;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else {
        info.code = (const Uint8 *)post_vert_msl;
        info.code_size = post_vert_msl_len;
        info.entrypoint = "vs_post";
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    }
    return SDL_CreateGPUShader(device, &info);
}

static SDL_GPUTexture *MakeTexture(SDL_GPUDevice *device,
                                   SDL_GPUTextureFormat format,
                                   SDL_GPUTextureUsageFlags usage) {
    SDL_GPUTextureCreateInfo info;

    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = usage;
    info.width = SIZE;
    info.height = SIZE;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(device, &info);
}

/* Left half 500 units away, right half 560, as the scene shader reads it. */
static int UploadStepDepth(SDL_GPUDevice *device, SDL_GPUTexture *depth,
                           const float depths[2]) {
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo source;
    SDL_GPUTextureRegion region;
    float *mapped;
    int x, y;

    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = SIZE * SIZE * sizeof(float);
    transfer = SDL_CreateGPUTransferBuffer(device, &info);
    if (transfer == NULL) return 0;
    mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped == NULL) return 0;
    for (y = 0; y < SIZE; y++)
        for (x = 0; x < SIZE; x++) mapped[y * SIZE + x] = depths[x >= SIZE / 2];
    SDL_UnmapGPUTransferBuffer(device, transfer);
    command = SDL_AcquireGPUCommandBuffer(device);
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(source);
    source.transfer_buffer = transfer;
    SDL_zero(region);
    region.texture = depth;
    region.w = SIZE;
    region.h = SIZE;
    region.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &region, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return 1;
}

/* Runs the effect over a mid-grey colour target and reads its red channel. */
static int Shade(SDL_GPUDevice *device, SDL_GPUTexture *color,
                 SDL_GPUTexture *depth, uint8_t out[SIZE * SIZE]) {
    SDL_GPUCommandBuffer *command = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUColorTargetInfo target;
    SDL_GPURenderPass *pass;
    SDL_GPUCopyPass *copy;
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUTextureRegion region;
    SDL_GPUTextureTransferInfo destination;
    ModernSsaoView view = {{1.0f, 1.0f, 0.0f, 100.0f}};
    const uint8_t *pixels;
    int i;

    SDL_zero(target);
    target.texture = color;
    target.clear_color = (SDL_FColor){0.5f, 0.5f, 0.5f, 1.0f};
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    pass = SDL_BeginGPURenderPass(command, &target, 1, NULL);
    SDL_EndGPURenderPass(pass);
    ModernSsaoApply(command, color, depth, &view);
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = SIZE * SIZE * 4;
    transfer = SDL_CreateGPUTransferBuffer(device, &info);
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(region);
    region.texture = color;
    region.w = SIZE;
    region.h = SIZE;
    region.d = 1;
    SDL_zero(destination);
    destination.transfer_buffer = transfer;
    SDL_DownloadFromGPUTexture(copy, &region, &destination);
    SDL_EndGPUCopyPass(copy);
    {
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
    }
    pixels = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (pixels == NULL) return 0;
    for (i = 0; i < SIZE * SIZE; i++) out[i] = pixels[i * 4];
    SDL_UnmapGPUTransferBuffer(device, transfer);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return 1;
}

int main(void) {
    SDL_GPUDevice *device;
    SDL_GPUShader *vertex;
    SDL_GPUTexture *color, *depth;
    static uint8_t shaded[SIZE * SIZE];
    /* ndc depth = 100 / view depth, with the view above. */
    const float step[2] = {100.0f / 500.0f, 100.0f / 560.0f};
    const float flat[2] = {100.0f / 500.0f, 100.0f / 500.0f};
    int cycle, row = SIZE / 2;

    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (device == NULL) { SDL_Quit(); return 77; }
    vertex = FullscreenVertex(device);
    CHECK(vertex);

    /* It can be built and torn down again and again, as the panel does. */
    CHECK(!ModernSsaoReady());
    CHECK(!ModernSsaoCreate(device, vertex, 0, SIZE) && !ModernSsaoReady());
    for (cycle = 0; cycle < 20; cycle++) {
        CHECK(ModernSsaoCreate(device, vertex, SIZE, SIZE) && ModernSsaoReady());
        ModernSsaoDestroy();
        CHECK(!ModernSsaoReady());
    }
    ModernSsaoApply(NULL, NULL, NULL, NULL); /* nothing to do, no crash */

    CHECK(ModernSsaoCreate(device, vertex, SIZE, SIZE));
    color = MakeTexture(device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                        SDL_GPU_TEXTUREUSAGE_SAMPLER);
    depth = MakeTexture(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                        SDL_GPU_TEXTUREUSAGE_SAMPLER);
    CHECK(color && depth);

    /* A flat wall is not occluded by itself. */
    if (!UploadStepDepth(device, depth, flat)) { puts("depth upload unsupported"); return 77; }
    CHECK(Shade(device, color, depth, shaded));
    CHECK(shaded[row * SIZE + 20] >= 124 && shaded[row * SIZE + 20] <= 132);
    CHECK(shaded[row * SIZE + SIZE / 2] >= 124 && shaded[row * SIZE + SIZE / 2] <= 132);

    /* Beside a nearer surface the farther one darkens, only near the step. */
    CHECK(UploadStepDepth(device, depth, step));
    CHECK(Shade(device, color, depth, shaded));
    CHECK(shaded[row * SIZE + SIZE / 2 + 2] + 6 < shaded[row * SIZE + SIZE - 8]);
    CHECK(shaded[row * SIZE + SIZE - 8] >= 124);
    CHECK(shaded[row * SIZE + 8] >= 124);

    ModernSsaoDestroy();
    SDL_ReleaseGPUTexture(device, color);
    SDL_ReleaseGPUTexture(device, depth);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    puts("ssao holds");
    return 0;
}
