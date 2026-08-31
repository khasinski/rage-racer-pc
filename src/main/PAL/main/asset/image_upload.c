#include "game/menu.h"
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

void UploadImageBlock(GameImageAssetHeaderWord *asset) {
    GameImageBlock *block;
    GameImageAssetAddress address;
    s32 flags;

    asset++;
    flags = asset->flags;
    asset++;

    if (flags & 8) {
        address.words = asset;
        block = address.block;
        UploadBlockPixels(block);
        asset = address.words + (block->size >> 2);
    }

    address.words = asset;
    block = address.block;
    if (block->w > 0 && block->h > 0) UploadBlockPixels(block);
}

/*
 * An image asset is a chain of [size][payload] links, ending at a size of
 * zero or less. The size word is read first and skipped past, so a block
 * starts at the word after its own header.
 */
void UploadImageAsset(void *asset) {
    GameImageAssetHeaderWord *ptr = asset;

    ptr++;
    for (;;) {
        s32 size = ptr->size;
        GameImageAssetHeaderWord *next;

        ptr++;
        if (size <= 0) return;
        next = ptr + ((u32)size >> 2);
        UploadImageBlock(ptr);
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
    UploadImageAsset(g_LoadBuffer);
}
