#ifndef RAGE_MODERN_ASSETS_H
#define RAGE_MODERN_ASSETS_H

#include "render/render_world.h"
#include "render/rmesh_cache.h"

void ModernAssetsInit(void);
void ModernAssetsShutdown(void);
const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance);
int ModernAssetsReady(void);
uint32_t ModernAssetsCachedMeshCount(void);
const RageRuntimeMesh *ModernAssetsMeshLookup(
    void *context, const RageRenderMeshInstance *instance);
int ModernAssetsLoadMaterialPixels(const RageRenderMeshInstance *instance,
                                   uint32_t material, const void **bytes,
                                   size_t *size);
void ModernAssetsFreeMaterialPixels(const void *bytes);
void ModernAssetsWarmWorld(const RageRenderWorld *world);

#endif

