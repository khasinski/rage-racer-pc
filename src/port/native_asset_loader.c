/* Native asset loader for the extracted retail RAGE.BIN archive. */
#include <stdio.h>

#include "game/asset.h"
#include "game/asset_internal.h"
#include "mod_assets.h"
#include "rage/compat.h"

void ResetAssetLoader(void) {
    g_CdLoadPhase = 0;
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
}

s32 EnableCdAudioMode(void) {
    return 1;
}

s32 LoadAsset(s32 assetIndex, void *dst) {
    s32 loaded;

    if ((u32)assetIndex >= GAME_ASSET_COUNT) return 0;
    loaded = ModAssetLoad((int)assetIndex, dst,
                          g_AssetCdEntries[assetIndex].size);
    if (loaded <= 0) {
        loaded = HostLoadAsset(
            g_AssetCdEntries[assetIndex].position.sectorOffset,
            g_AssetCdEntries[assetIndex].size, dst);
    }
    /* Edited images are applied to the asset in memory, so a mod can carry
     * PNGs alone and the directory it lives in is never written to. */
    if (loaded > 0) {
        ModPatchTextures((int)assetIndex, dst, (size_t)loaded);
        return loaded;
    }
    return 0;
}

void LoadDiscArchiveIndex(void) {
    if (!HostLoadArchiveIndex(g_AssetCdEntries, GAME_ASSET_COUNT)) {
        printf("Unable to load assets/PAL/RAGE.BIN\n");
    }
}

void InitAssetSystem(void) {
    LoadDiscArchiveIndex();
    if (LoadAsset(ASSET_BOOT_LOGO, g_LoadBuffer) > 0) {
        UploadImageAsset(GetImageAssetHeaderWords(g_LoadBuffer));
    }
}

s32 RequestBootAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_BOOT, 1, 0);
}
