#include "game/menu_internal.h"
#include "game/prim.h"
#include <stdio.h>

typedef struct Tile { s32 x, y, w, h; u8 r, g, b; } Tile;
static Tile tiles[2048];
static unsigned count;
static int bad;
static s32 height;

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                s32 w, s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    if (count >= 2048 || x < 7 || y < 9 || w <= 0 || h <= 0 ||
        x + w > 119 || y + h > 9 + height) { bad = 1; return packet; }
    tiles[count++] = (Tile){x - 7, y - 9, w, h, (u8)r, (u8)g, (u8)b};
    return packet + 1;
}

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    u8 packets[2048];
    for (int item = 0; item < TITLE_MENU_ITEM_COUNT; ++item) {
        for (int selected = 0; selected <= 1; ++selected) {
            for (height = 1; height <= 16; ++height) {
                count = 0; bad = 0;
                u8 *end = DrawTitleMenuLabel(NULL, packets, (TitleMenuItem)item,
                                             7, 9, height, selected);
                CHECK(!bad && count && end == packets + count);
                const Tile *panel = &tiles[count - 1];
                CHECK(panel->x == 0 && panel->y == 0 && panel->w == 112 && panel->h == height);
                if (height == 16) CHECK(count > 1);
            }
        }
    }
    count = 0;
    CHECK(DrawTitleMenuLabel(NULL, packets, (TitleMenuItem)-1, 7, 9, 16, 0) == packets);
    CHECK(DrawTitleMenuLabel(NULL, packets, TITLE_MENU_ITEM_COUNT, 7, 9, 16, 0) == packets);
    CHECK(DrawTitleMenuLabel(NULL, packets, TITLE_MENU_OPTIONS, 7, 9, 0, 0) == packets);
    CHECK(count == 0);
    puts("title labels stay within their revealed panels in both states");
    return 0;
}
