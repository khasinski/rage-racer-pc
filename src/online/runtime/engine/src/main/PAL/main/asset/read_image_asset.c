#include "game/image_asset.h"
#include <string.h>

enum { IMAGE_VRAM_WIDTH = 1024, IMAGE_VRAM_HEIGHT = 512 };

/* Asset bytes may come from an unaligned archive slice. Read only metadata;
 * validation neither uploads pixels nor changes the caller's storage. */
static s32 ImageBlockFits(const void *data, size_t size) {
    GameImageBlock block;
    const size_t headerSize = offsetof(GameImageBlock, pixels);
    if (size < headerSize) return 0;
    memcpy(&block, data, headerSize);
    size_t pixelCount = (size_t)block.w * block.h;
    return pixelCount <= (size - headerSize) / sizeof(u16) &&
           (block.w == 0 || (size_t)block.x + block.w <= IMAGE_VRAM_WIDTH) &&
           (block.h == 0 || (size_t)block.y + block.h <= IMAGE_VRAM_HEIGHT);
}

s32 IsValidImageEntry(const void *entry, size_t size) {
    GameImageEntryHeader header;
    const size_t blockHeaderSize = offsetof(GameImageBlock, pixels);
    if (entry == NULL || size < sizeof(header) + blockHeaderSize) return 0;
    memcpy(&header, entry, sizeof(header));
    const u8 *cursor = (const u8 *)(const void *)entry + sizeof(header);
    size_t remaining = size - sizeof(header);
    if ((header.flags & GAME_IMAGE_ENTRY_HAS_CLUT) != 0) {
        u32 clutSize;
        memcpy(&clutSize, cursor, sizeof(clutSize));
        if (clutSize < blockHeaderSize || (clutSize & 3u) != 0 ||
            clutSize > remaining || !ImageBlockFits(cursor, clutSize)) return 0;
        cursor += clutSize;
        remaining -= clutSize;
    }
    return ImageBlockFits(cursor, remaining);
}

/* The first word is followed by [size][entry] links and a nonpositive size. */
s32 IsValidImageAsset(const void *asset, size_t size) {
    if (asset == NULL || size < sizeof(GameImageAssetHeaderWord)) return 0;
    const u8 *cursor = (const u8 *)(const void *)asset + sizeof(GameImageAssetHeaderWord);
    size_t remaining = size - sizeof(GameImageAssetHeaderWord);
    while (remaining >= sizeof(s32)) {
        s32 entrySize;
        memcpy(&entrySize, cursor, sizeof(entrySize));
        cursor += sizeof(entrySize);
        remaining -= sizeof(entrySize);
        if (entrySize <= 0) return 1;
        if ((u32)entrySize > remaining || ((u32)entrySize & 3u) != 0 ||
            !IsValidImageEntry((const void *)cursor, (size_t)entrySize)) return 0;
        cursor += entrySize;
        remaining -= (size_t)entrySize;
    }
    return 0;
}

static void CopyBlockPixels(const u8 *data, const ImagePixels *image) {
    GameImageBlock block;
    const size_t headerSize = offsetof(GameImageBlock, pixels);
    memcpy(&block, data, headerSize);
    u32 left = block.x > image->x ? block.x : image->x;
    u32 top = block.y > image->y ? block.y : image->y;
    u32 right = (u32)block.x + block.w;
    u32 bottom = (u32)block.y + block.h;
    if (right > image->x + image->width) right = image->x + image->width;
    if (bottom > image->y + image->height) bottom = image->y + image->height;
    if (right <= left || bottom <= top) return;
    for (u32 y = top; y < bottom; ++y) {
        size_t destination = (size_t)(y - image->y) * image->width + left - image->x;
        size_t source = (size_t)(y - block.y) * block.w + left - block.x;
        memcpy(image->words + destination, data + headerSize + source * sizeof(u16),
               (size_t)(right - left) * sizeof(u16));
    }
}

static int ValidDestination(const ImagePixels *image) {
    return image && image->words && image->width && image->height &&
        image->width <= IMAGE_VRAM_WIDTH && image->height <= IMAGE_VRAM_HEIGHT &&
        image->x <= IMAGE_VRAM_WIDTH - image->width &&
        image->y <= IMAGE_VRAM_HEIGHT - image->height &&
        image->count >= (size_t)image->width * image->height;
}

static void CopyEntryPixels(const u8 *entry, const ImagePixels *image) {
    GameImageEntryHeader header;
    memcpy(&header, entry, sizeof(header));
    const u8 *block = entry + sizeof(header);
    if ((header.flags & GAME_IMAGE_ENTRY_HAS_CLUT) != 0) {
        u32 clutSize;
        memcpy(&clutSize, block, sizeof(clutSize));
        CopyBlockPixels(block, image);
        block += clutSize;
    }
    CopyBlockPixels(block, image);
}

s32 CopyImageEntryPixels(const void *entry, size_t size, const ImagePixels *image) {
    if (!ValidDestination(image) || !IsValidImageEntry(entry, size)) return 0;
    CopyEntryPixels(entry, image);
    return 1;
}

s32 CopyImageAssetPixels(const void *asset, size_t size, const ImagePixels *image) {
    if (!ValidDestination(image) || !IsValidImageAsset(asset, size)) return 0;
    const u8 *cursor = (const u8 *)asset + sizeof(GameImageAssetHeaderWord);
    for (;;) {
        s32 entrySize;
        memcpy(&entrySize, cursor, sizeof(entrySize));
        cursor += sizeof(entrySize);
        if (entrySize <= 0) return 1;
        CopyEntryPixels(cursor, image);
        cursor += entrySize;
    }
}
