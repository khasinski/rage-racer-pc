#include "game/memcard.h"

#include <stdio.h>

void BuildSaveIconBlock(u8 *block, const char *title, s32 iconTile,
                        s32 imageX, s32 imageY) {
    Rect rect;

    block[0] = 'S';
    block[1] = 'C';
    block[2] = 0x11;
    block[3] = 1;
    snprintf((char *)block + MC_ICON_TITLE_OFS,
             MC_ICON_CLUT_OFS - MC_ICON_TITLE_OFS,
             g_FmtString, title);

    rect.x = (iconTile % 20) * 16;
    rect.y = iconTile / 20 + 0x1E0;
    rect.w = 0x10;
    rect.h = 1;
    StoreImage(&rect, block + MC_ICON_CLUT_OFS);
    DrawSync(0);

    rect.x = imageX;
    rect.y = imageY;
    rect.w = 4;
    rect.h = 0x10;
    StoreImage(&rect, block + MC_ICON_PIXELS_OFS);
    DrawSync(0);
}
