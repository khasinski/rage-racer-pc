#include "game/menu_internal.h"
#include "game/prim.h"
#include "menu_images.h"
#include "menu_label_font.h"
#include "content/title_image.h"
#include <string.h>

enum {
    CUSTOM_MENU_TEXTURE_WIDTH = 112,
    CUSTOM_MENU_TEXTURE_HEIGHT = 16,
    CUSTOM_MENU_BACKGROUND_INDEX = 4,
};

static u8 ColorComponent(u16 color, s32 shift) {
    return ((color >> shift) & 0x1F) << 3;
}

/* AddPrim prepends, so the bar is queued after the ink and ends up behind it. */
static u8 *EmitFontLabel(GameOrderingTableEntry *ot, u8 *packet,
                         s32 x, s32 y, s32 visibleHeight,
                         const uint8_t *pixels) {
    const uint8_t *bar = pixels + 2 * 3;
    s32 row;

    for (row = 0; row < visibleHeight; row++) {
        const uint8_t *line = pixels + row * MENU_LABEL_WIDTH * 3;
        s32 column = 0;

        while (column < MENU_LABEL_WIDTH) {
            const uint8_t *pixel = line + column * 3;
            s32 start = column;
            int plate = pixel[0] == bar[0] && pixel[1] == bar[1] &&
                        pixel[2] == bar[2];

            column++;
            while (column < MENU_LABEL_WIDTH &&
                   memcmp(pixel, line + column * 3, 3) == 0) {
                column++;
            }
            if (plate) continue;
            packet = AddTilePrim(ot, packet, x + start, y + row,
                                 column - start, 1,
                                 pixel[0], pixel[1], pixel[2]);
        }
    }
    return AddTilePrim(ot, packet, x, y, MENU_LABEL_WIDTH, visibleHeight,
                       bar[0], bar[1], bar[2]);
}

u8 *DrawTitleMenuLabel(GameOrderingTableEntry *ot, u8 *packet,
                       TitleMenuItem item, s32 x, s32 y,
                       s32 visibleHeight, s32 selected) {
    u16 background = TitleImageColor(selected,CUSTOM_MENU_BACKGROUND_INDEX);
    s32 row;

    if (item < 0 || item >= TITLE_MENU_ITEM_COUNT || visibleHeight <= 0)
        return packet;
    if (visibleHeight > CUSTOM_MENU_TEXTURE_HEIGHT)
        visibleHeight = CUSTOM_MENU_TEXTURE_HEIGHT;

    const u8 *image = MenuImage(item, selected);
    const uint8_t *fontPixels;

    if (image) {
        for(row=0;row<visibleHeight;row++)for(s32 column=0;column<CUSTOM_MENU_TEXTURE_WIDTH;){
            const u8 *pixel=image+(row*CUSTOM_MENU_TEXTURE_WIDTH+column)*4;
            s32 start=column++;
            while(column<CUSTOM_MENU_TEXTURE_WIDTH&&!memcmp(pixel,image+(row*CUSTOM_MENU_TEXTURE_WIDTH+column)*4,4))column++;
            if(pixel[3]>=128)packet=AddTilePrim(ot,packet,x+start,y+row,column-start,1,pixel[0],pixel[1],pixel[2]);
        }
        return packet;
    }

    fontPixels = MenuLabelFontPixels(item, selected);
    if (fontPixels != NULL) {
        return EmitFontLabel(ot, packet, x, y, visibleHeight, fontPixels);
    }

    for (row = 0; row < visibleHeight; row++) {
        s32 column = 0;

        while (column < CUSTOM_MENU_TEXTURE_WIDTH) {
            u8 index = TitleImageIndex(item,column,row);
            s32 start = column++;
            u16 color;

            while (column < CUSTOM_MENU_TEXTURE_WIDTH &&
                   TitleImageIndex(item,column,row) == index) {
                column++;
            }
            if (index == CUSTOM_MENU_BACKGROUND_INDEX) continue;
            color = TitleImageColor(selected,index);
            packet = AddTilePrim(ot, packet, x + start, y + row,
                                 column - start, 1,
                                 ColorComponent(color, 0),
                                 ColorComponent(color, 5),
                                 ColorComponent(color, 10));
        }
    }

    /* AddPrim prepends to the ordering table. Queue the panel last so it is
     * executed first and the previously queued glyph runs draw over it. */
    packet = AddTilePrim(ot, packet, x, y, CUSTOM_MENU_TEXTURE_WIDTH,
                         visibleHeight, ColorComponent(background, 0),
                         ColorComponent(background, 5),
                         ColorComponent(background, 10));

    return packet;
}
