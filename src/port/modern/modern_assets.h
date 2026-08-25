#ifndef RAGE_MODERN_ASSETS_H
#define RAGE_MODERN_ASSETS_H

#include "render/render_world.h"
#include "render/rmesh_cache.h"

typedef struct ModernAssetMaterialSource {
    uint32_t tpage;
    uint32_t clut;
    uint32_t windowWidthU;
    uint32_t windowWidthV;
    uint32_t windowOffsetU;
    uint32_t windowOffsetV;
} ModernAssetMaterialSource;

void ModernAssetsInit(void);
void ModernAssetsShutdown(void);
const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance);
int ModernAssetsReady(void);
uint32_t ModernAssetsCachedMeshCount(void);
const RageRuntimeMesh *ModernAssetsMeshLookup(
    void *context, const RageRenderMeshInstance *instance);
int ModernAssetsLoadMaterialPixels(const RageRenderMeshInstance *instance,
                                   uint32_t material, uint8_t variant,
                                   const void **bytes,
                                   size_t *size,
                                   ModernAssetMaterialSource *source);
void ModernAssetsFreeMaterialPixels(const void *bytes);
void ModernAssetsWarmWorld(const RageRenderWorld *world);

#endif
