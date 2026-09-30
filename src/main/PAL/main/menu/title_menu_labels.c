#include "game/menu_internal.h"
#include "game/prim.h"

enum {
    CUSTOM_MENU_TEXTURE_WIDTH = 112,
    CUSTOM_MENU_TEXTURE_HEIGHT = 16,
    CUSTOM_MENU_BACKGROUND_INDEX = 4,
};

static const u32 s_TitleMenuTextures[] = {
#include "assets/title_menu_textures.inc"
};

static const u32 s_CustomMenuTexture[] = {
#include "assets/custom_menu_texture.inc"
};

_Static_assert(sizeof(s_TitleMenuTextures) ==
                   5 * CUSTOM_MENU_TEXTURE_WIDTH *
                       CUSTOM_MENU_TEXTURE_HEIGHT / 2,
               "retail title menu textures must contain five rows");
_Static_assert(sizeof(s_CustomMenuTexture) ==
                   CUSTOM_MENU_TEXTURE_WIDTH * CUSTOM_MENU_TEXTURE_HEIGHT / 2,
               "custom menu texture must fill one 112x16 4-bit row");

static const u16 s_CustomMenuPalettes[2][16] = {
    {0x8000, 0xCE73, 0xC631, 0xC210, 0xBDEF, 0xB9CE, 0xB5AD, 0xB18C,
     0xA529, 0xA108, 0x98C6, 0x94A5, 0x9084, 0x8C63, 0x8842, 0x8421},
    {0x8000, 0xB9DF, 0xB19D, 0xA97C, 0xA55A, 0xA139, 0x9D18, 0x98F6,
     0x9092, 0x8C70, 0x884D, 0x842B, 0x8429, 0x8006, 0x8001, 0x8000},
};

static u8 PixelIndex(const u32 *texture, s32 x, s32 y) {
    u32 word = texture[(y * CUSTOM_MENU_TEXTURE_WIDTH + x) / 8];
    return (word >> ((x & 7) * 4)) & 0xF;
}

static u8 ColorComponent(u16 color, s32 shift) {
    return ((color >> shift) & 0x1F) << 3;
}

u8 *DrawTitleMenuLabel(GameOrderingTableEntry *ot, u8 *packet,
                       TitleMenuItem item, s32 x, s32 y,
                       s32 visibleHeight, s32 selected) {
    const u16 *palette = s_CustomMenuPalettes[selected != 0];
    const u32 *texture;
    u16 background = palette[CUSTOM_MENU_BACKGROUND_INDEX];
    s32 row;

    if (item < 0 || item >= TITLE_MENU_ITEM_COUNT || visibleHeight <= 0)
        return packet;
    if (item == TITLE_MENU_CUSTOM) {
        texture = s_CustomMenuTexture;
    } else {
        s32 retailItem = item > TITLE_MENU_CUSTOM ? item - 1 : item;
        texture = s_TitleMenuTextures +
                  retailItem * CUSTOM_MENU_TEXTURE_WIDTH *
                      CUSTOM_MENU_TEXTURE_HEIGHT / 8;
    }
    if (visibleHeight > CUSTOM_MENU_TEXTURE_HEIGHT)
        visibleHeight = CUSTOM_MENU_TEXTURE_HEIGHT;

    for (row = 0; row < visibleHeight; row++) {
        s32 column = 0;

        while (column < CUSTOM_MENU_TEXTURE_WIDTH) {
            u8 index = PixelIndex(texture, column, row);
            s32 start = column++;
            u16 color;

            while (column < CUSTOM_MENU_TEXTURE_WIDTH &&
                   PixelIndex(texture, column, row) == index) {
                column++;
            }
            if (index == CUSTOM_MENU_BACKGROUND_INDEX) continue;
            color = palette[index];
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
