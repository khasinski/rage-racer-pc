#ifndef GAME_IMAGE_ASSET_H
#define GAME_IMAGE_ASSET_H
#include "common.h"
#include <stddef.h>

/* One VRAM upload record inside an image entry. UploadImageAsset walks the
 * outer entry chain; UploadImageEntry uploads its optional CLUT and pixels. */
typedef struct GameImageBlock {
    u32 size;   /* +0x00 block size in bytes, rounded down to a word */
    u16 x;      /* +0x04 VRAM destination */
    u16 y;      /* +0x06 */
    u16 w;      /* +0x08 in 16-bit words */
    u16 h;      /* +0x0A */
    u8 pixels[4]; /* +0x0C */
} GameImageBlock;

typedef union GameImageAssetHeaderWord {
    s32 size;
    s32 flags;
} GameImageAssetHeaderWord;

typedef struct GameImageEntryHeader {
    s32 reserved;
    u32 flags;
} GameImageEntryHeader;

enum {
    GAME_IMAGE_ENTRY_HAS_CLUT = 1 << 3
};

static inline const GameImageAssetHeaderWord *GetImageAssetHeaderWords(
    const void *data) {
    return (const GameImageAssetHeaderWord *)data;
}

static inline const GameImageEntryHeader *GetImageEntryHeader(
    const void *data) {
    return (const GameImageEntryHeader *)data;
}

s32 IsValidImageEntry(const void *entry, size_t size);
s32 IsValidImageAsset(const void *asset, size_t size);
/* Caller-owned rectangle in retail texture coordinates. Only its intersecting
 * pixels are copied; everything outside the asset's blocks remains unchanged.
 * Source and destination storage must not overlap. */
typedef struct ImagePixels {
    u16 *words;
    size_t count;
    u32 x, y, width, height;
} ImagePixels;

/* Validates the entire asset/destination before changing any output pixels. */
s32 CopyImageEntryPixels(const void *entry, size_t size, const ImagePixels *image);
s32 CopyImageAssetPixels(const void *asset, size_t size, const ImagePixels *image);
#endif
