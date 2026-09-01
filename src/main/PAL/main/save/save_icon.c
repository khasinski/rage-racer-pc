#include "game/memcard.h"
#include "game/menu.h"

void BuildSaveIconBlock(u8 *block, char *title, s32 iconTile,
                        s32 imageX, s32 imageY) {
    Rect *rect = &g_SaveIconRect;

    block[0] = 'S';
    block[1] = 'C';
    block[2] = 0x11;
    block[3] = 1;
    sprintf((char *)block + 4, g_FmtString, title);

    rect->x = (iconTile % 20) * 16;
    rect->y = iconTile / 20 + 0x1E0;
    rect->w = 0x10;
    rect->h = 1;
    StoreImage(rect, block + 0x60);
    DrawSync(0);

    rect->x = imageX;
    rect->y = imageY;
    rect->w = 4;
    rect->h = 0x10;
    StoreImage(rect, block + 0x80);
    DrawSync(0);
}
