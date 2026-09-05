#ifndef RAGE_NATIVE_ASSET_IMPORTER_H
#define RAGE_NATIVE_ASSET_IMPORTER_H

#include <stdint.h>

#include "modern/modern_assets.h"
#include "render/rmesh_cache.h"
#include "track_texture_snapshot.h"

/* Runtime import boundary for the retail PS1 assets. The importer consumes
 * the game's currently loaded model/image data and exposes conventional
 * renderer-native meshes and RGBA images. The modern renderer never needs to
 * understand model banks, VRAM pages, CLUTs or texture windows. */
int NativeAssetImporterInit(void);
void NativeAssetImporterShutdown(void);
int NativeAssetImporterReady(void);
/* Retain only already-captured pixels of the requested generation; no live
 * read/import occurs here. Caller releases the returned reference. */
RageTrackTextureGeneration *NativeAssetImporterRetainTextures(uint64_t revision);
/* Borrowed until importer shutdown. Meshes are session-resident, not evicted
 * on a track revision: prepared/captured frames may still reference them. */
const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RageRenderMeshInstance *instance);
uint32_t NativeAssetImporterMeshCount(void);
int NativeAssetImporterLoadMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image);
int NativeAssetImporterLoadSky(uint32_t assetKey,
    const RageSkyPanoramaLayout *layout, ModernAssetImage *image);

#endif
