#include "game/asset.h"


void InstallCourseAssets(void) {
    InstallTrackTextureAssetPack(g_AssetBase);
}

s32 RequestTrackDataAssets(void) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == ASSET_REQUEST_TRACK_DATA) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    g_AssetRequestType = ASSET_REQUEST_TRACK_DATA;
    g_AssetLoadState = 1;
    return 1;
}
