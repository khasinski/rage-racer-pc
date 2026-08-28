#ifndef RAGE_NATIVE_ASSET_IMPORTER_H
#define RAGE_NATIVE_ASSET_IMPORTER_H

#include <stdint.h>

#include "modern/modern_assets.h"
#include "render/rmesh_cache.h"

/* Runtime import boundary for the retail PS1 assets. The importer consumes
 * the game's currently loaded model/image data and exposes conventional
 * renderer-native meshes and RGBA images. The modern renderer never needs to
 * understand model banks, VRAM pages, CLUTs or texture windows. */
int RageNativeAssetImporterInit(void);
void RageNativeAssetImporterShutdown(void);
int RageNativeAssetImporterReady(void);
const RageRuntimeCachedMesh *RageNativeAssetImporterFind(
    const RageRenderMeshInstance *instance);
uint32_t RageNativeAssetImporterMeshCount(void);
int RageNativeAssetImporterLoadMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image);
int RageNativeAssetImporterLoadSky(uint32_t assetKey, ModernAssetImage *image);

#endif
