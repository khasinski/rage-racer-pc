#include "modern_vram_snapshot.h"

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
