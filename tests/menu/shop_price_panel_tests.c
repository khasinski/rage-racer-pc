#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
GameRenderState g_RenderState;

typedef struct DrawRecord {
    s32 kind;
    s32 y;
    s32 width;
    s32 textureU;
    s32 value;
} DrawRecord;

static DrawRecord s_records[8];
static s32 s_recordCount;

s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 r, u8 g, u8 b,
                   u16 clut, u8 texturePageOffset) {
    (void)x;
    (void)flags;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)texturePageOffset;
    s_records[s_recordCount++] = (DrawRecord){1, y, 0, 0, (s32)value};
    return flags;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)height;
    (void)textureV;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_records[s_recordCount++] = (DrawRecord){2, y, width, textureU, 0};
}

void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height, u8 r, u8 g,
                        u8 b) {
    (void)x;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_records[s_recordCount++] = (DrawRecord){3, y, 0, 0, 0};
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int CheckPanel(void (*draw)(s32, s32, s32), s32 captionWidth,
                      s32 captionU) {
    s_recordCount = 0;
    draw(0, 100, 50);
    CHECK(s_recordCount == 0);

    draw(11, 100, 50);
    CHECK(s_recordCount == 0);

    draw(1, 100, 50);
    CHECK(s_recordCount == 8);
    CHECK(s_records[0].kind == 1 && s_records[0].y == 502 &&
          s_records[0].value == 100);
    CHECK(s_records[1].kind == 1 && s_records[1].y == 542 &&
          s_records[1].value == 50);
    CHECK(s_records[3].kind == 2 && s_records[3].width == captionWidth &&
          s_records[3].textureU == captionU);

    s_recordCount = 0;
    draw(SHOP_PANEL_SLIDE_MAX, 100, 50);
    s_recordCount = 0;
    draw(1, 100, 50);
    CHECK(s_records[0].y == 152);
    CHECK(s_records[1].y == 192);
    CHECK(s_records[6].kind == 3 && s_records[6].y == 142);
    CHECK(s_records[7].kind == 3 && s_records[7].y == 182);

    return 0;
}

int main(void) {
    static GameOrderingTableEntry orderingTable[1];

    RENDER_OT_BASE = orderingTable;
    CHECK(CheckPanel(DrawCarShopPricePanel, 0x18, 0x3C) == 0);
    CHECK(CheckPanel(DrawEngineerShopPricePanel, 0x34, 0x54) == 0);

    s_recordCount = 0;
    RENDER_OT_BASE = NULL;
    DrawCarShopPricePanel(0, 100, 50);
    DrawCarShopPricePanel(SHOP_PANEL_VISIBLE_AT, 100, 50);
    DrawCarShopPricePanel(1, 100, 50);
    CHECK(s_recordCount == 0);
    puts("shop price panel tests passed");
    return 0;
}
