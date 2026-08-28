#include "game/menu.h"
#include "game/race.h"
#include "game/asset.h"


void UploadImageBlock(GameImageAssetHeaderWord *asset) {
    GameImageBlock *block;
    GameImageAssetAddress address;
    Rect rect;
    s32 flags;

    asset++;
    flags = asset->flags;
    asset++;

    if (flags & 8) {
        address.words = asset;
        block = address.block;
        rect.x = block->x;
        rect.y = block->y;
        rect.w = block->w;
        rect.h = block->h;
        LoadImage(&rect, block->pixels);
        DrawSync(0);
        address.block = block;
        asset = address.words + (block->size >> 2);
    }

    address.words = asset;
    block = address.block;
    rect.x = block->x;
    rect.y = block->y;
    rect.w = block->w;
    rect.h = block->h;
    if ((rect.w > 0) && (rect.h > 0)) {
        LoadImage(&rect, block->pixels);
        DrawSync(0);
    }
}

/*
 * Walk the chain of image blocks in an image asset. Each link is a word count
 * followed by that many bytes of GameImageBlock records; a word <= 0 ends it.
 *
 * The jump into the loop is load-bearing, not decompiler residue: every plain
 * `while` / `for (;;) { ...; if (x) break; }` spelling lets gcc 2.6.3's
 * duplicate_loop_exit_test copy the test above the loop, which costs 12
 * instructions retail does not have.
 */
void UploadImageAsset(void *asset) {
    GameImageAssetHeaderWord *ptr;
    s32 size;

    ptr = asset;
    ptr++;
    goto test;

    do {
        u32 blockSize = size;
        GameImageAssetHeaderWord *next = ptr + (blockSize >> 2);
        UploadImageBlock(ptr);
        ptr = next;
    test:
        size = ptr->size;
        ptr++;
    } while (size > 0);
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
