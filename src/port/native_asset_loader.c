/* Native asset loader for the extracted retail RAGE.BIN archive. */
#include "mod_assets.h"
#include <stdio.h>
#include "game/asset.h"
#include "game/render_internal.h"
#include "rage/compat.h"

void SetTrackCameraTable(void *table) {
    g_TrackRenderTable = table;
}

void ResetAssetLoader(void) {
    g_CdLoadPhase = 0;
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
}

s32 EnableCdAudioMode(void) {
    return 1;
}

s32 LoadAsset(s32 assetIndex, void *dst) {
    s32 loaded = ModAssetLoad((int)assetIndex, dst,
                                  g_AssetCdEntries[assetIndex].size);
    if (loaded == 0)
        loaded = HostLoadAsset(g_AssetCdEntries[assetIndex].position.sectorOffset,
                                   g_AssetCdEntries[assetIndex].size, dst);
    /* Edited images are applied to the asset in memory, so a mod can carry
     * PNGs alone and the directory it lives in is never written to. */
    if (loaded != 0) ModPatchTextures((int)assetIndex, dst, (size_t)loaded);
    return loaded;
}

void LoadAssetBlocking(s32 assetIndex, void *dst) {
    LoadAsset(assetIndex, dst);
}

void LoadDiscArchiveIndex(void) {
    if (!HostLoadArchiveIndex(g_AssetCdEntries, 135)) {
        printf("Unable to load assets/PAL/RAGE.BIN\n");
    }
}

void InitAssetSystem(void) {
    LoadDiscArchiveIndex();
    LoadAssetBlocking(0, &g_LoadBuffer);
    UploadImageAsset(&g_LoadBuffer);
}

s32 RequestBootAssets(void) {
    if (g_AssetLoadState != 0) return 1;
    if (g_AssetRequestType == ASSET_REQUEST_BOOT) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }
    g_AssetRequestType = ASSET_REQUEST_BOOT;
    g_AssetLoadState = 1;
    return 1;
}
