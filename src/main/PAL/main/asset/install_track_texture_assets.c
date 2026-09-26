#include "game/asset.h"
#include "game/track_images.h"
#include "game/asset_internal.h"
#include "rage/track_asset_identity.h"

static void ClearTrackTextureAssetPack(void) {
    g_TrackTextureShadow = NULL;
    g_AssetLoadCursor = NULL;
}

s32 InstallTrackTextureAssetPack(u8 *base, size_t size) {
    TrackTextureAssetView view;

    if (!ReadTrackImages(base, size, &view)) {
        ClearTrackTextureAssetPack();
        return 0;
    }

    if (!UploadImageAsset(
            GetImageAssetHeaderWords(
                view.blocks[TRACK_TEXTURE_PRIMARY_IMAGES]),
            view.sizes[TRACK_TEXTURE_PRIMARY_IMAGES]) ||
        !UploadImageAsset(
            GetImageAssetHeaderWords(
                view.blocks[TRACK_TEXTURE_SECONDARY_IMAGES]),
            view.sizes[TRACK_TEXTURE_SECONDARY_IMAGES]) ||
        !UploadImageEntry(
            GetImageEntryHeader(view.blocks[TRACK_TEXTURE_CAR_IMAGE]),
            view.sizes[TRACK_TEXTURE_CAR_IMAGE]) ||
        !UploadImageAsset(
            GetImageAssetHeaderWords(
                view.blocks[TRACK_TEXTURE_ACTIVE_IMAGES]),
            view.sizes[TRACK_TEXTURE_ACTIVE_IMAGES])) {
        ClearTrackTextureAssetPack();
        return 0;
    }

    /* Preserve page 1 before the deferred upload replaces the same VRAM
     * rectangle with page 0. Publishing the shadow can wait until both
     * uploads succeed, but taking its copy cannot. */
    StoreTeamLogoImage(base);
    if (!UploadImageAsset(
            GetImageAssetHeaderWords(
                view.blocks[TRACK_TEXTURE_DEFERRED_IMAGES]),
            view.sizes[TRACK_TEXTURE_DEFERRED_IMAGES])) {
        ClearTrackTextureAssetPack();
        return 0;
    }

    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
    /* Texture data and its CLUTs become resident before the matching runtime
     * pack publishes its asset id.  Give the native renderer a new generation
     * now so it cannot retain RGBA textures decoded from the preceding scene
     * (or from an earlier load of this same course). */
    TrackAssetIdentityInvalidate();
    return 1;
}

s32 InstallTrackPreviewTexturePack(u8 *base, size_t size) {
    TrackTextureAssetView view;

    if (!ReadTrackImages(base, size, &view)) return 0;
    return UploadImageAsset(
               GetImageAssetHeaderWords(
                   view.blocks[TRACK_TEXTURE_PRIMARY_IMAGES]),
               view.sizes[TRACK_TEXTURE_PRIMARY_IMAGES]) &&
           UploadImageAsset(
               GetImageAssetHeaderWords(
                   view.blocks[TRACK_TEXTURE_SECONDARY_IMAGES]),
               view.sizes[TRACK_TEXTURE_SECONDARY_IMAGES]) &&
           UploadImageEntry(
               GetImageEntryHeader(view.blocks[TRACK_TEXTURE_CAR_IMAGE]),
               view.sizes[TRACK_TEXTURE_CAR_IMAGE]) &&
           UploadImageAsset(
               GetImageAssetHeaderWords(
                   view.blocks[TRACK_TEXTURE_ACTIVE_IMAGES]),
               view.sizes[TRACK_TEXTURE_ACTIVE_IMAGES]) &&
           UploadImageAsset(
               GetImageAssetHeaderWords(
                   view.blocks[TRACK_TEXTURE_DEFERRED_IMAGES]),
               view.sizes[TRACK_TEXTURE_DEFERRED_IMAGES]);
}
