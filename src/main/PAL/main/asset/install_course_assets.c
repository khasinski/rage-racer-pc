#include "game/asset.h"
#include "game/asset_internal.h"


void InstallCourseAssets(void) {
    InstallTrackTextureAssetPack(g_AssetBase);
}

s32 RequestTrackDataAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_TRACK_DATA, 1, 0);
}
