#include "game/asset.h"


void InstallCourseAssets(void) {
    u8 *base;
    s32 offset0;
    s32 offset1;

    g_AssetBlockPtr = g_AssetBase + GetSceneAssetHeader(g_AssetBase)->offsets[0];
    UploadImageAsset(g_AssetBlockPtr);

    g_AssetBlockPtr = g_AssetBase + GetSceneAssetHeader(g_AssetBase)->offsets[1];
    UploadImageAsset(g_AssetBlockPtr);

    g_AssetBlockPtr = g_AssetBase + GetSceneAssetHeader(g_AssetBase)->offsets[2];
    UploadImageBlock(GetImageAssetHeaderWords(g_AssetBlockPtr));

    base = g_AssetBase;
    offset0 = GetSceneAssetHeader(base)->offsets[3];
    offset1 = GetSceneAssetHeader(base)->offsets[4];
    g_AssetBlockPtr = base + offset0;
    g_AssetSubBlockPtr = base + offset1;
    UploadImageAsset(g_AssetBlockPtr);

    StoreTeamLogoImage(g_AssetBase);

    g_TrackTextureShadow = GetTrackTextureShadowRows(g_AssetBase);
    UploadImageAsset(g_AssetSubBlockPtr);
    ResetTrackTextureSwap();
    g_AssetLoadCursor = g_AssetBase + TRACK_TEXTURE_SHADOW_SIZE;
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
