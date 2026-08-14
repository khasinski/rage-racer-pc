/* Native asset loader for the extracted retail RAGE.BIN archive. */
#include "common.h"
#include <stdio.h>
#include "game/state.h"
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
    return RageHostLoadAsset(g_AssetCdEntries[assetIndex].position.sectorOffset,
                             g_AssetCdEntries[assetIndex].size, dst);
}

void LoadAssetBlocking(s32 assetIndex, void *dst) {
    LoadAsset(assetIndex, dst);
}

void LoadDiscArchiveIndex(void) {
    if (!RageHostLoadArchiveIndex(g_AssetCdEntries, 135)) {
        printf("Unable to load assets/PAL/RAGE.BIN\n");
    }
}

void InitAssetSystem(void) {
    void *ptr;

    LoadDiscArchiveIndex();
    ptr = &g_LoadBuffer;
    LoadAssetBlocking(0, ptr);
    UploadImageAsset(ptr);
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
