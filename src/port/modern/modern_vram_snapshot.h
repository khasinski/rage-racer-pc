#ifndef RAGE_MODERN_VRAM_SNAPSHOT_H
#define RAGE_MODERN_VRAM_SNAPSHOT_H

#include <SDL3/SDL_gpu.h>
#include <stdint.h>

typedef SDL_GPUTexture *(*ModernVramSnapshotCapture)(void *context);

typedef struct ModernVramSnapshotCache {
    uint32_t frame;
    uint64_t trackRevision, assetGeneration;
    SDL_GPUTexture *texture;
    int valid;
} ModernVramSnapshotCache;

/* Read-only lookup: never captures from live VRAM on a miss. */
SDL_GPUTexture *ModernVramSnapshotLookup(
    const ModernVramSnapshotCache *cache, uint32_t frame,
    uint64_t trackRevision, uint64_t assetGeneration);

SDL_GPUTexture *ModernVramSnapshotForFrame(
    ModernVramSnapshotCache *cache, uint32_t frame,
    uint64_t trackRevision, uint64_t assetGeneration,
    ModernVramSnapshotCapture capture, void *context);
/* Forget borrowed GPU handles when their device/resources are retired. */
void ModernVramSnapshotReset(ModernVramSnapshotCache *cache);

/* Copy native RGBA8 1024x512 VRAM into caller-owned sampling storage. The
 * caller releases *owned with the same device; subsequent copies cycle its
 * backing storage so already submitted draws retain their original contents. */
int ModernVramSnapshotCopy(SDL_GPUDevice *device, SDL_GPUTexture *source,
                           SDL_GPUTexture **owned);

#endif
