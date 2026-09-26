#include "game/asset.h"
#include "game/race.h"

#include <stddef.h>

static void UploadBlockPixels(const GameImageBlock *block) {
    Rect rect;

    rect.x = block->x;
    rect.y = block->y;
    rect.w = block->w;
    rect.h = block->h;
    LoadImage(&rect, (void *)(const void *)block->pixels);
    DrawSync(0);
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
