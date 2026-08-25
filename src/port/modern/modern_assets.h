#ifndef RAGE_MODERN_ASSETS_H
#define RAGE_MODERN_ASSETS_H

#include "render/render_world.h"
#include "render/rmesh_cache.h"

typedef struct ModernAssetImage {
    void *pixels;
    size_t size;
    uint32_t width;
    uint32_t height;
} ModernAssetImage;

void ModernAssetsInit(void);
void ModernAssetsShutdown(void);
const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance);
int ModernAssetsReady(void);
uint32_t ModernAssetsCachedMeshCount(void);
const RageRuntimeMesh *ModernAssetsMeshLookup(
    void *context, const RageRenderMeshInstance *instance);
int ModernAssetsLoadMaterialImage(const RageRenderMeshInstance *instance,
                                  uint32_t material, uint8_t variant,
                                  ModernAssetImage *image);
void ModernAssetsFreeMaterialImage(ModernAssetImage *image);
void ModernAssetsWarmWorld(const RageRenderWorld *world);

#endif
