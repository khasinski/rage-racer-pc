#include "game/asset.h"

enum {
    TRACK_TEXTURE_PRIMARY_IMAGES = 0,
    TRACK_TEXTURE_SECONDARY_IMAGES = 1,
    TRACK_TEXTURE_CAR_IMAGE = 2,
    TRACK_TEXTURE_ACTIVE_IMAGES = 3,
    TRACK_TEXTURE_DEFERRED_IMAGES = 4,
};

typedef struct TrackTextureAssetHeader {
    s32 offsets[5];
} TrackTextureAssetHeader;

s32 InstallTrackTextureAssetPack(u8 *base, size_t size) {
    TrackTextureAssetHeader *header;
    void *primaryImages;
    void *secondaryImages;
    void *carImage;
    void *activeImages;
    void *deferredImages;
    s32 i;

    if (base == NULL || size < sizeof(*header) || size > INT32_MAX) return 0;

    header = (TrackTextureAssetHeader *)base;
    for (i = 0; i < 5; i++) {
        s32 end = i + 1 < 5 ? header->offsets[i + 1] : (s32)size;

        if (header->offsets[i] < (s32)sizeof(*header) ||
            end <= header->offsets[i] || (size_t)end > size) {
            return 0;
        }
    }

    primaryImages = base + header->offsets[TRACK_TEXTURE_PRIMARY_IMAGES];
    secondaryImages = base + header->offsets[TRACK_TEXTURE_SECONDARY_IMAGES];
    carImage = base + header->offsets[TRACK_TEXTURE_CAR_IMAGE];
    activeImages = base + header->offsets[TRACK_TEXTURE_ACTIVE_IMAGES];
    deferredImages = base + header->offsets[TRACK_TEXTURE_DEFERRED_IMAGES];

    UploadImageAsset(GetImageAssetHeaderWords(primaryImages));
    UploadImageAsset(GetImageAssetHeaderWords(secondaryImages));
    UploadImageEntry(GetImageEntryHeader(carImage));
    UploadImageAsset(GetImageAssetHeaderWords(activeImages));
    StoreTeamLogoImage(base);
    g_TrackTextureShadow = GetTrackTextureShadowRows(base);
    UploadImageAsset(GetImageAssetHeaderWords(deferredImages));
    ResetTrackTextureSwap();
    g_AssetLoadCursor = base + TRACK_TEXTURE_SHADOW_SIZE;
    return 1;
}
