#include "game/memcard.h"

#include <stdio.h>

void BuildSaveIconBlock(GameSaveIconBlock *block, const char *title) {
    enum {
        SAVE_ICON_TILE = 0x222,
        SAVE_ICON_IMAGE_X = 0x3C0,
        SAVE_ICON_IMAGE_Y = 0x1F0,
    };
    Rect rect;

    block->magic[0] = 'S';
    block->magic[1] = 'C';
    block->format = 0x11;
    block->frameCount = 1;
    snprintf(block->title, sizeof(block->title), "%s", title);

    rect.x = (SAVE_ICON_TILE % 20) * 16;
    rect.y = SAVE_ICON_TILE / 20 + 0x1E0;
    rect.w = 0x10;
    rect.h = 1;
    StoreImage(&rect, block->clut);
    DrawSync(0);

    rect.x = SAVE_ICON_IMAGE_X;
    rect.y = SAVE_ICON_IMAGE_Y;
    rect.w = 4;
    rect.h = 0x10;
    StoreImage(&rect, block->pixels);
    DrawSync(0);
}
