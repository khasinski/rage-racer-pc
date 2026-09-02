#include "game/asset.h"

enum {
    TRACK_TEXTURE_PRIMARY_IMAGES = 0,
    TRACK_TEXTURE_SECONDARY_IMAGES = 1,
    TRACK_TEXTURE_CAR_IMAGE = 2,
    TRACK_TEXTURE_ACTIVE_IMAGES = 3,
    TRACK_TEXTURE_DEFERRED_IMAGES = 4,
};

void InstallTrackTextureAssetPack(u8 *base) {
    GameSceneAssetHeader *header = GetSceneAssetHeader(base);

    g_AssetBlockPtr = GetSceneAssetAddress(
        header, header->offsets[TRACK_TEXTURE_PRIMARY_IMAGES]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(
        header, header->offsets[TRACK_TEXTURE_SECONDARY_IMAGES]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(
        header, header->offsets[TRACK_TEXTURE_CAR_IMAGE]);
    UploadImageEntry(GetImageEntryHeader(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(
        header, header->offsets[TRACK_TEXTURE_ACTIVE_IMAGES]);
    g_AssetSubBlockPtr = GetSceneAssetAddress(
        header, header->offsets[TRACK_TEXTURE_DEFERRED_IMAGES]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    StoreTeamLogoImage(base);
    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetSubBlockPtr));
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
}
