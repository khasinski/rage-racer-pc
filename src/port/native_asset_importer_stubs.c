#include "native_asset_importer.h"

#include <stddef.h>

#include "game/track.h"

/* The generated 512x128 sky asset stores the eight source tiles. Offline
 * render tools need the retail map to arrange those tiles into the same two
 * authored rows as the live importer, but otherwise carry no game state. */
s32 g_SkyRowBase;
s16 g_SkyTileMap[SKY_TILE_MAP_ROWS][SKY_TILE_MAP_COLUMNS] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3},
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3},
};

/* Offline render tools consume an already-generated native asset cache and do
 * not link the game state needed by the live retail-disc importer. */
int NativeAssetImporterInit(void) {
    return 0;
}

void NativeAssetImporterShutdown(void) {
}

int NativeAssetImporterReady(void) {
    return 0;
}

const RageRuntimeCachedMesh *NativeAssetImporterFind(
    const RageRenderMeshInstance *instance) {
    (void)instance;
    return NULL;
}

uint32_t NativeAssetImporterMeshCount(void) {
    return 0;
}

int NativeAssetImporterLoadMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image) {
    (void)instance;
    (void)material;
    (void)variant;
    (void)definition;
    (void)image;
    return 0;
}

int NativeAssetImporterLoadSky(uint32_t assetKey,
                                   ModernAssetImage *image) {
    (void)assetKey;
    (void)image;
    return 0;
}
