#include "game/asset.h"

void InstallTrackTextureAssetPack(u8 *base) {
    GameSceneAssetHeader *header = GetSceneAssetHeader(base);

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[0]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[1]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[2]);
    UploadImageEntry(GetImageEntryHeader(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[3]);
    g_AssetSubBlockPtr = GetSceneAssetAddress(header, header->offsets[4]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    StoreTeamLogoImage(base);
    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetSubBlockPtr));
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
}
