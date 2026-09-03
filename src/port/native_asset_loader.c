/* Native asset loader for the extracted retail RAGE.BIN archive. */
#include <stdio.h>
#include <string.h>

#include "game/asset.h"
#include "game/asset_internal.h"
#include "archive_index.h"
#include "mod_assets.h"
#include "rage/compat.h"

_Static_assert(sizeof(GameCdLoadEntry) == sizeof(RageArchiveIndexEntry),
               "host and game archive entries must have the same layout");
_Static_assert(offsetof(GameCdLoadEntry, size) ==
                   offsetof(RageArchiveIndexEntry, size),
               "host and game archive sizes must share an offset");

void ResetAssetLoader(void) {
    g_CdLoadPhase = 0;
    g_AssetLoadState = 0;
    g_AssetLoadFailed = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
}

s32 EnableCdAudioMode(void) {
    return 1;
}

s32 LoadAsset(s32 assetIndex, void *dst) {
    s32 loaded;
    size_t room;

    if ((u32)assetIndex >= GAME_ASSET_COUNT || dst == NULL) return -1;
    room = PortAssetRoomAt(dst);
    if (room == 0 || g_AssetCdEntries[assetIndex].size > room) return -1;
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
    return -1;
}

void LoadDiscArchiveIndex(void) {
    if (!HostLoadArchiveIndex(g_AssetCdEntries, GAME_ASSET_COUNT)) {
        memset(g_AssetCdEntries, 0, sizeof(g_AssetCdEntries));
        g_AssetLoadFailed = 1;
        fprintf(stderr, "rage-port: unable to load the disc archive index\n");
    }
}

void InitAssetSystem(void) {
    s32 loadedSize;

    LoadDiscArchiveIndex();
    if (g_AssetLoadFailed) return;
    loadedSize = LoadAsset(ASSET_BOOT_LOGO, g_LoadBuffer);
    if (loadedSize > 0) {
        UploadImageAsset(GetImageAssetHeaderWords(g_LoadBuffer),
                         (size_t)loadedSize);
    }
}

s32 RequestBootAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_BOOT, 1, 0);
}
