#include "game/asset.h"
#include "game/race.h"

#include <stddef.h>

enum {
    IMAGE_VRAM_WIDTH = 1024,
    IMAGE_VRAM_HEIGHT = 512,
};

static void UploadBlockPixels(const GameImageBlock *block) {
    Rect rect;

    rect.x = block->x;
    rect.y = block->y;
    rect.w = block->w;
    rect.h = block->h;
    LoadImage(&rect, (void *)(const void *)block->pixels);
    DrawSync(0);
}

static s32 ImageBlockFits(const GameImageBlock *block, size_t size) {
    size_t pixelCount;
    size_t pixelBytes;

    if (size < offsetof(GameImageBlock, pixels)) return 0;
    pixelCount = (size_t)block->w * block->h;
    if (pixelCount > (SIZE_MAX - offsetof(GameImageBlock, pixels)) /
                         sizeof(u16)) {
        return 0;
    }
    pixelBytes = pixelCount * sizeof(u16);
    return pixelBytes <= size - offsetof(GameImageBlock, pixels) &&
           (block->w == 0 ||
            (size_t)block->x + block->w <= IMAGE_VRAM_WIDTH) &&
           (block->h == 0 ||
            (size_t)block->y + block->h <= IMAGE_VRAM_HEIGHT);
}

s32 IsValidImageEntry(const GameImageEntryHeader *entry, size_t size) {
    const GameImageBlock *clut = NULL;
    const GameImageBlock *pixels;
    const u8 *cursor;
    size_t remaining;

    if (entry == NULL || size < sizeof(*entry) +
                                      offsetof(GameImageBlock, pixels)) {
        return 0;
    }

    cursor = (const u8 *)(entry + 1);
    remaining = size - sizeof(*entry);
    pixels = (const GameImageBlock *)(const void *)cursor;

    if ((entry->flags & GAME_IMAGE_ENTRY_HAS_CLUT) != 0) {
        clut = pixels;
        if (clut->size < offsetof(GameImageBlock, pixels) ||
            (clut->size & (sizeof(u32) - 1)) != 0 ||
            clut->size > remaining ||
            !ImageBlockFits(clut, (size_t)clut->size)) {
            return 0;
        }
        cursor += clut->size;
        remaining -= clut->size;
        pixels = (const GameImageBlock *)(const void *)cursor;
    }

    return ImageBlockFits(pixels, remaining);
}

s32 UploadImageEntry(const GameImageEntryHeader *entry, size_t size) {
    const GameImageBlock *clut = NULL;
    const GameImageBlock *pixels;

    if (!IsValidImageEntry(entry, size)) return 0;

    pixels = (const GameImageBlock *)(entry + 1);
    if ((entry->flags & GAME_IMAGE_ENTRY_HAS_CLUT) != 0) {
        clut = pixels;
        pixels = (const GameImageBlock *)((const u8 *)pixels + clut->size);
    }

    if (clut != NULL) UploadBlockPixels(clut);
    if (pixels->w > 0 && pixels->h > 0) {
        UploadBlockPixels(pixels);
    }
    return 1;
}

/*
 * An image asset is a chain of [size][payload] links, ending at a size of
 * zero or less. The size word is read first and skipped past, so a block
 * starts at the word after its own header.
 */
s32 IsValidImageAsset(const GameImageAssetHeaderWord *asset, size_t size) {
    const u8 *cursor;
    size_t remaining;

    if (asset == NULL || size < sizeof(*asset)) return 0;

    cursor = (const u8 *)(asset + 1);
    remaining = size - sizeof(*asset);
    while (remaining >= sizeof(GameImageAssetHeaderWord)) {
        s32 entrySize =
            ((const GameImageAssetHeaderWord *)(const void *)cursor)->size;

        cursor += sizeof(GameImageAssetHeaderWord);
        remaining -= sizeof(GameImageAssetHeaderWord);
        if (entrySize <= 0) return 1;
        if ((u32)entrySize > remaining ||
            (u32)entrySize < sizeof(GameImageEntryHeader) +
                                 offsetof(GameImageBlock, pixels) ||
            ((u32)entrySize & (sizeof(GameImageAssetHeaderWord) - 1)) != 0 ||
            !IsValidImageEntry(
                (const GameImageEntryHeader *)(const void *)cursor,
                (size_t)entrySize)) {
            return 0;
        }
        cursor += entrySize;
        remaining -= (size_t)entrySize;
    }
    return 0;
}

s32 UploadImageAsset(const GameImageAssetHeaderWord *asset, size_t size) {
    const u8 *cursor;

    if (!IsValidImageAsset(asset, size)) return 0;

    cursor = (const u8 *)(asset + 1);
    for (;;) {
        s32 entrySize =
            ((const GameImageAssetHeaderWord *)(const void *)cursor)->size;

        cursor += sizeof(GameImageAssetHeaderWord);
        if (entrySize <= 0) return 1;
        UploadImageEntry(GetImageEntryHeader(cursor), (size_t)entrySize);
        cursor += entrySize;
    }
}

void StoreTeamLogoImage(void *dst) {
    g_TeamLogoClut[0] = CLUT_STP_BIT;
    LoadImage(&g_TeamLogoClutLoadRect, g_TeamLogoClut);

    if (g_GrandPrixSeries != 0) {
        MoveImage(&g_TeamLogoClutMoveRect, 0x3F0, 0xE2);
    }

    StoreImage(&g_TrackTextureRect, dst);
    DrawSync(0);
    g_TeamLogoClut[0] = 0;
}

s32 UploadLoadBufferImage(void) {
    return UploadImageAsset(GetImageAssetHeaderWords(g_LoadBuffer),
                            g_LoadBufferImageSize);
}
