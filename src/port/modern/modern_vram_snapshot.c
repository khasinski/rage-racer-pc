#include "modern_vram_snapshot.h"

int ModernVramSnapshotCopy(SDL_GPUDevice *device, SDL_GPUTexture *source,
                           SDL_GPUTexture **owned) {
    if (!device || !source || !owned) return 0;
    if (!*owned) {
        const SDL_GPUTextureCreateInfo info = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = 1024, .height = 512, .layer_count_or_depth = 1,
            .num_levels = 1,
        };
        *owned = SDL_CreateGPUTexture(device, &info);
        if (!*owned) return 0;
    }
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return 0;
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) {
        SDL_CancelGPUCommandBuffer(cmd);
        return 0;
    }
    const SDL_GPUTextureLocation from = {.texture = source};
    const SDL_GPUTextureLocation to = {.texture = *owned};
    SDL_CopyGPUTextureToTexture(copy, &from, &to, 1024, 512, 1, true);
    SDL_EndGPUCopyPass(copy);
    return SDL_SubmitGPUCommandBuffer(cmd);
}

void ModernVramSnapshotReset(ModernVramSnapshotCache *cache) {
    if (cache != NULL) *cache = (ModernVramSnapshotCache){0};
}

SDL_GPUTexture *ModernVramSnapshotForFrame(
    ModernVramSnapshotCache *cache, uint32_t frame,
    ModernVramSnapshotCapture capture, void *context) {
    SDL_GPUTexture *texture;

    if (cache == NULL || capture == NULL) return NULL;
    if (cache->valid && cache->frame == frame && cache->texture != NULL)
        return cache->texture;
    texture = capture(context);
    if (texture == NULL) return NULL;
    cache->frame = frame;
    cache->texture = texture;
    cache->valid = 1;
    return texture;
}
