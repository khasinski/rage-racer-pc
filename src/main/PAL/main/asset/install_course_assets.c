#include "game/asset.h"


void InstallCourseAssets(void) {
    u8 *base;
    GameSceneAssetHeader *header;

    base = g_AssetBase;
    header = GetSceneAssetHeader(base);

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[0]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[1]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[2]);
    UploadImageBlock(GetImageAssetHeaderWords(g_AssetBlockPtr));

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[3]);
    g_AssetSubBlockPtr = GetSceneAssetAddress(header, header->offsets[4]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));

    StoreTeamLogoImage(base);

    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetSubBlockPtr));
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
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
