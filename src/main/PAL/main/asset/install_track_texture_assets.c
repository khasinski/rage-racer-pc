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
    void *primaryImages = GetSceneAssetBlock(
        header, TRACK_TEXTURE_PRIMARY_IMAGES);
    void *secondaryImages = GetSceneAssetBlock(
        header, TRACK_TEXTURE_SECONDARY_IMAGES);
    void *carImage = GetSceneAssetBlock(header, TRACK_TEXTURE_CAR_IMAGE);
    void *activeImages = GetSceneAssetBlock(
        header, TRACK_TEXTURE_ACTIVE_IMAGES);
    void *deferredImages = GetSceneAssetBlock(
        header, TRACK_TEXTURE_DEFERRED_IMAGES);

    UploadImageAsset(GetImageAssetHeaderWords(primaryImages));
    UploadImageAsset(GetImageAssetHeaderWords(secondaryImages));
    UploadImageEntry(GetImageEntryHeader(carImage));
    UploadImageAsset(GetImageAssetHeaderWords(activeImages));
    StoreTeamLogoImage(base);
    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    UploadImageAsset(GetImageAssetHeaderWords(deferredImages));
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
}
