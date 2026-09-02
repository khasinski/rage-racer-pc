#include "game/menu_internal.h"
#include "game/race.h"
#include "game/asset.h"

static void UploadBlockPixels(GameImageBlock *block) {
    Rect rect;

    rect.x = block->x;
    rect.y = block->y;
    rect.w = block->w;
    rect.h = block->h;
    LoadImage(&rect, block->pixels);
    DrawSync(0);
}

void UploadImageEntry(GameImageEntryHeader *entry) {
    GameImageBlock *block;

    if (entry == NULL) return;

    block = (GameImageBlock *)(entry + 1);

    if ((entry->flags & GAME_IMAGE_ENTRY_HAS_CLUT) != 0) {
        if (block->size < sizeof(*block) ||
            (block->size & (sizeof(u32) - 1)) != 0) {
            return;
        }
        UploadBlockPixels(block);
        block = (GameImageBlock *)((u8 *)block + block->size);
    }

    if (block->w > 0 && block->h > 0) {
        UploadBlockPixels(block);
    }
}

/*
 * An image asset is a chain of [size][payload] links, ending at a size of
 * zero or less. The size word is read first and skipped past, so a block
 * starts at the word after its own header.
 */
void UploadImageAsset(GameImageAssetHeaderWord *asset) {
    GameImageAssetHeaderWord *ptr;

    if (asset == NULL) return;

    ptr = asset + 1;
    for (;;) {
        s32 size = ptr->size;
        GameImageAssetHeaderWord *next;

        ptr++;
        if (size <= 0) return;
        if ((u32)size < sizeof(GameImageEntryHeader) +
                            sizeof(GameImageBlock) ||
            ((u32)size & (sizeof(*ptr) - 1)) != 0) {
            return;
        }
        next = ptr + ((u32)size >> 2);
        UploadImageEntry(GetImageEntryHeader(ptr));
        ptr = next;
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

void UploadLoadBufferImage(void) {
    UploadImageAsset(GetImageAssetHeaderWords(g_LoadBuffer));
}
