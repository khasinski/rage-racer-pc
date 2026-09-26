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
struct CarModelData;
/* Import a canonical human model before its first cache use. Copies source
 * storage; later material decoding does not depend on active slots or VRAM.
 * Owned sources coexist with legacy keys; conflicting owned bytes are rejected. */
int NativeAssetImporterPrepareCar(uint32_t variant, const struct CarModelData *model);
struct RaceView;
/* Call during race asset preparation, before submitting a field. Failure
 * removes only entries added by this call and preserves the existing cache. */
int NativeAssetImporterPrepareRaceView(const struct RaceView *view);
/* Retain only already-captured pixels of the requested generation; no live
 * read/import occurs here. Caller releases the returned reference. */
RageTrackTextureGeneration *NativeAssetImporterRetainTextures(uint64_t revision);
/* Borrowed until importer shutdown. Meshes are session-resident, not evicted
 * on a track revision: prepared/captured frames may still reference them. */
int NativeAssetImporterMaterialSlot(const RenderMeshInstance *instance,
    uint16_t tpage, uint16_t clut);
const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RenderMeshInstance *instance);
uint32_t NativeAssetImporterMeshCount(void);
const RageRuntimeCachedMesh *NativeAssetImporterPeek(uint32_t assetKey, RenderAssetSet assetSet, RenderAssetSource source);
int NativeAssetImporterLoadMaterial(
    const RenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image);
int NativeAssetImporterLoadSky(uint32_t assetKey,
    const RageSkyPanoramaLayout *layout, ModernAssetImage *image);
/* Team artwork is live game state, even when the base atlas came from disk. */
int NativeAssetImporterApplyPlayerMarkings(uint16_t clut, ModernAssetImage *image);

#endif
