#include "game/asset.h"
#include "game/asset_internal.h"
#include "rage/track_asset_identity.h"

enum {
    TRACK_TEXTURE_PRIMARY_IMAGES = 0,
    TRACK_TEXTURE_SECONDARY_IMAGES = 1,
    TRACK_TEXTURE_CAR_IMAGE = 2,
    TRACK_TEXTURE_ACTIVE_IMAGES = 3,
    TRACK_TEXTURE_DEFERRED_IMAGES = 4,
    TRACK_TEXTURE_BLOCK_COUNT = 5,
};

typedef struct TrackTextureAssetHeader {
    s32 offsets[TRACK_TEXTURE_BLOCK_COUNT];
} TrackTextureAssetHeader;

typedef struct TrackTextureAssetView {
    u8 *blocks[TRACK_TEXTURE_BLOCK_COUNT];
    size_t sizes[TRACK_TEXTURE_BLOCK_COUNT];
} TrackTextureAssetView;

static void ClearTrackTextureAssetPack(void) {
    g_TrackTextureShadow = NULL;
    g_AssetLoadCursor = NULL;
}

static s32 ResolveTrackTextureAssetPack(u8 *base, size_t size,
                                        TrackTextureAssetView *view) {
    const TrackTextureAssetHeader *header;
    s32 i;

    if (base == NULL || view == NULL || size < TRACK_TEXTURE_SHADOW_SIZE ||
        size > INT32_MAX) {
        return 0;
    }

    header = (const TrackTextureAssetHeader *)(const void *)base;
    if (header->offsets[TRACK_TEXTURE_DEFERRED_IMAGES] <
        TRACK_TEXTURE_SHADOW_SIZE) {
        return 0;
    }
    for (i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; i++) {
        s32 start = header->offsets[i];
        s32 end = i + 1 < TRACK_TEXTURE_BLOCK_COUNT
                      ? header->offsets[i + 1]
                      : (s32)size;

        if (start < (s32)sizeof(*header) || end <= start ||
            (size_t)end > size) {
            return 0;
        }
        view->blocks[i] = base + start;
        view->sizes[i] = (size_t)(end - start);
    }

    if (!IsValidImageAsset(
            GetImageAssetHeaderWords(view->blocks[TRACK_TEXTURE_PRIMARY_IMAGES]),
            view->sizes[TRACK_TEXTURE_PRIMARY_IMAGES]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(
                view->blocks[TRACK_TEXTURE_SECONDARY_IMAGES]),
            view->sizes[TRACK_TEXTURE_SECONDARY_IMAGES]) ||
        !IsValidImageEntry(
            GetImageEntryHeader(view->blocks[TRACK_TEXTURE_CAR_IMAGE]),
            view->sizes[TRACK_TEXTURE_CAR_IMAGE]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(view->blocks[TRACK_TEXTURE_ACTIVE_IMAGES]),
            view->sizes[TRACK_TEXTURE_ACTIVE_IMAGES]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(
                view->blocks[TRACK_TEXTURE_DEFERRED_IMAGES]),
            view->sizes[TRACK_TEXTURE_DEFERRED_IMAGES])) {
        return 0;
    }
    return 1;
}

s32 InstallTrackTextureAssetPack(u8 *base, size_t size) {
    TrackTextureAssetView view;

    if (!ResolveTrackTextureAssetPack(base, size, &view)) {
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
            view.sizes[TRACK_TEXTURE_ACTIVE_IMAGES]) ||
        !UploadImageAsset(
            GetImageAssetHeaderWords(
                view.blocks[TRACK_TEXTURE_DEFERRED_IMAGES]),
            view.sizes[TRACK_TEXTURE_DEFERRED_IMAGES])) {
        ClearTrackTextureAssetPack();
        return 0;
    }

    StoreTeamLogoImage(base);
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
