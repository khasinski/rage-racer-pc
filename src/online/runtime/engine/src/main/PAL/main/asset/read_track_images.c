#include "game/track_images.h"
#include <string.h>
#include <stdlib.h>

int ReadTrackImages(const void *data, size_t size,
                                        TrackTextureAssetView *view) {
    const u8 *base = data;
    TrackTextureAssetHeader header;
    TrackTextureAssetView candidate;
    s32 i;

    if (base == NULL || view == NULL || size < TRACK_TEXTURE_SHADOW_SIZE ||
        size > INT32_MAX) {
        return 0;
    }

    memcpy(&header, base, sizeof(header));
    if (header.offsets[TRACK_TEXTURE_DEFERRED_IMAGES] <
        TRACK_TEXTURE_SHADOW_SIZE) {
        return 0;
    }
    for (i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; i++) {
        s32 start = header.offsets[i];
        s32 end = i + 1 < TRACK_TEXTURE_BLOCK_COUNT
                      ? header.offsets[i + 1]
                      : (s32)size;

        if (start < (s32)sizeof(header) || end <= start ||
            (size_t)end > size) {
            return 0;
        }
        candidate.blocks[i] = base + start;
        candidate.sizes[i] = (size_t)(end - start);
    }

    if (!IsValidImageAsset(
            GetImageAssetHeaderWords(candidate.blocks[TRACK_TEXTURE_PRIMARY_IMAGES]),
            candidate.sizes[TRACK_TEXTURE_PRIMARY_IMAGES]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(
                candidate.blocks[TRACK_TEXTURE_SECONDARY_IMAGES]),
            candidate.sizes[TRACK_TEXTURE_SECONDARY_IMAGES]) ||
        !IsValidImageEntry(
            GetImageEntryHeader(candidate.blocks[TRACK_TEXTURE_CAR_IMAGE]),
            candidate.sizes[TRACK_TEXTURE_CAR_IMAGE]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(candidate.blocks[TRACK_TEXTURE_ACTIVE_IMAGES]),
            candidate.sizes[TRACK_TEXTURE_ACTIVE_IMAGES]) ||
        !IsValidImageAsset(
            GetImageAssetHeaderWords(
                candidate.blocks[TRACK_TEXTURE_DEFERRED_IMAGES]),
            candidate.sizes[TRACK_TEXTURE_DEFERRED_IMAGES])) {
        return 0;
    }
    *view = candidate;
    return 1;
}

TrackImages *CopyTrackImages(const void *data, size_t size) {
    TrackTextureAssetView view;
    if (!ReadTrackImages(data, size, &view) ||
        size > SIZE_MAX - sizeof(TrackImages)) return NULL;
    TrackImages *images = malloc(sizeof(*images) + size);
    if (!images) return NULL;
    memcpy(images->storage, data, size);
    /* Rebase validated views instead of parsing the immutable copy again. */
    for (s32 i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; ++i) {
        images->view.blocks[i] = images->storage +
            ((const u8 *)view.blocks[i] - (const u8 *)data);
        images->view.sizes[i] = view.sizes[i];
    }
    return images;
}

void FreeTrackImages(TrackImages *images) { free(images); }
