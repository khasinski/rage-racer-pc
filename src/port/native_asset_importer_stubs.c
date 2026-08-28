#include "native_asset_importer.h"

#include <stddef.h>

/* Offline render tools consume an already-generated native asset cache and do
 * not link the game state needed by the live retail-disc importer. */
int RageNativeAssetImporterInit(void) {
    return 0;
}

void RageNativeAssetImporterShutdown(void) {
}

int RageNativeAssetImporterReady(void) {
    return 0;
}

const RageRuntimeCachedMesh *RageNativeAssetImporterFind(
    const RageRenderMeshInstance *instance) {
    (void)instance;
    return NULL;
}

uint32_t RageNativeAssetImporterMeshCount(void) {
    return 0;
}

int RageNativeAssetImporterLoadMaterial(
    const RageRenderMeshInstance *instance, uint32_t material,
    uint8_t variant, RageRenderMaterial *definition, ModernAssetImage *image) {
    (void)instance;
    (void)material;
    (void)variant;
    (void)definition;
    (void)image;
    return 0;
}

int RageNativeAssetImporterLoadSky(uint32_t assetKey,
                                   ModernAssetImage *image) {
    (void)assetKey;
    (void)image;
    return 0;
}
