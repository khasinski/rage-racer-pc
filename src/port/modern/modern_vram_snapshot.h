#ifndef RAGE_MODERN_VRAM_SNAPSHOT_H
#define RAGE_MODERN_VRAM_SNAPSHOT_H

#include <SDL3/SDL_gpu.h>
#include <stdint.h>

typedef SDL_GPUTexture *(*ModernVramSnapshotCapture)(void *context);

typedef struct ModernVramSnapshotCache {
    uint32_t frame;
    SDL_GPUTexture *texture;
    int valid;
} ModernVramSnapshotCache;

SDL_GPUTexture *ModernVramSnapshotForFrame(
    ModernVramSnapshotCache *cache, uint32_t frame,
    ModernVramSnapshotCapture capture, void *context);

#endif
