#ifndef GAME_TRACK_IMAGES_H
#define GAME_TRACK_IMAGES_H
#include "game/image_asset.h"

#define TRACK_TEXTURE_SHADOW_SIZE 0x38000

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
    const void *blocks[TRACK_TEXTURE_BLOCK_COUNT];
    size_t sizes[TRACK_TEXTURE_BLOCK_COUNT];
} TrackTextureAssetView;

/* Borrows source storage; failure leaves the destination unchanged. */
int ReadTrackImages(const void *data, size_t size, TrackTextureAssetView *view);
/* Owns validated texture-pack bytes. Views point into storage; do not copy. */
typedef struct TrackImages {
    TrackTextureAssetView view;
    u8 storage[];
} TrackImages;
TrackImages *CopyTrackImages(const void *data, size_t size);
void FreeTrackImages(TrackImages *images);
#endif

