#include "game/memcard.h"

#include <stdio.h>

void BuildSaveIconBlock(GameSaveIconBlock *block, const char *title, s32 iconTile,
                        s32 imageX, s32 imageY) {
    Rect rect;

    block->magic[0] = 'S';
    block->magic[1] = 'C';
    block->format = 0x11;
    block->frameCount = 1;
    snprintf(block->title, sizeof(block->title), g_FmtString, title);

    rect.x = (iconTile % 20) * 16;
    rect.y = iconTile / 20 + 0x1E0;
    rect.w = 0x10;
    rect.h = 1;
    StoreImage(&rect, block->clut);
    DrawSync(0);

    rect.x = imageX;
    rect.y = imageY;
    rect.w = 4;
    rect.h = 0x10;
    StoreImage(&rect, block->pixels);
    DrawSync(0);
}
