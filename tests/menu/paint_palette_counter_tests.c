#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <limits.h>

PaintColorTable g_PaintColorTable;
s32 g_PaintPalettePulsePhase;
s32 g_MenuAltLayout;
s32 g_OwnedCarCounterSlide;
GameRenderState g_RenderState;

typedef struct RectRecord {
    s32 x;
    s32 y;
    s32 r;
    s32 g;
    s32 b;
} RectRecord;

static RectRecord s_solidRects[20];
static RectRecord s_outlines[2];
static s32 s_solidCount;
static s32 s_outlineCount;
static s32 s_spriteCount;
static s32 s_spriteY;
static s32 s_numberCount;
static s32 s_numberY;
static u32 s_firstNumber;
static s32 s_buttonCount;
static s32 s_buttonY;

s32 rsin(s32 angle) {
    (void)angle;
    return 4096;
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)width;
    (void)height;
    (void)alpha;
    s_solidRects[s_solidCount++] = (RectRecord){x, y, r, g, b};
}

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height, u8 r,
                     u8 g, u8 b, u8 alpha) {
    (void)ot;
    (void)width;
    (void)height;
    (void)alpha;
    s_outlines[s_outlineCount++] = (RectRecord){x, y, r, g, b};
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 u,
                u16 v, u8 r, u8 g, u8 b, u16 clut, s32 shade,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shade;
    (void)semiTrans;
    (void)flags;
    s_spriteCount++;
    s_spriteY = y;
}

s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 r, u8 g, u8 b,
                   u16 clut, u8 primitiveCount) {
    (void)x;
    (void)flags;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)primitiveCount;
    if (s_numberCount == 0) {
        s_numberY = y;
        s_firstNumber = value;
    }
    s_numberCount++;
    return 0;
}

void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height, u8 r, u8 g,
                        u8 b) {
    (void)x;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_buttonCount++;
    s_buttonY = y;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetDraws(void) {
    s_solidCount = 0;
    s_outlineCount = 0;
    s_spriteCount = 0;
    s_numberCount = 0;
    s_buttonCount = 0;
}

int main(void) {
    GameOrderingTableEntry orderingTable[1] = {0};
    s32 progress = 10;
    s32 i;

    g_RenderState.primData = orderingTable;
    for (i = 0; i < MENU_PAINT_COLOR_COUNT; i++) {
        g_PaintColorTable.colors[i] = (Rgb){i, i + 1, i + 2};
    }

    CHECK(DrawPaintColorPalette(&progress, 1, 3) == 0);
    CHECK(progress == 11 && s_solidCount == 0);
    CHECK(DrawPaintColorPalette(&progress, 1, 3) == 0);
    CHECK(progress == 12 && s_solidCount == 19 && s_outlineCount == 2);
    CHECK(s_outlines[0].x == 0xB4 && s_outlines[0].y == 0x20B);
    CHECK(s_solidRects[0].x == 0xB5 && s_solidRects[0].y == 0x20D);
    CHECK(s_solidRects[0].r == 3 && s_solidRects[0].g == 4);
    CHECK(s_solidRects[1].x == 0x9F && s_solidRects[18].x == 0x127);
    CHECK(g_PaintPalettePulsePhase == 0x20);

    ResetDraws();
    progress = 25;
    g_MenuAltLayout = 1;
    CHECK(DrawPaintColorPalette(&progress, 0, 17) == 1);
    CHECK(s_outlines[0].x == 0xF8 && s_outlines[0].y == 0x175);
    CHECK(progress == 25);

    ResetDraws();
    CHECK(DrawPaintColorPalette(NULL, 1, 0) == 0);
    progress = 12;
    CHECK(DrawPaintColorPalette(&progress, 0, -1) == 0);
    CHECK(s_solidRects[0].r == 0 && s_solidRects[0].g == 1);
    progress = INT_MAX;
    CHECK(DrawPaintColorPalette(&progress, INT_MAX, 0) == 1);
    CHECK(progress == 25);
    progress = INT_MIN;
    CHECK(DrawPaintColorPalette(&progress, -1, 0) == 0);
    CHECK(progress == 0);

    ResetDraws();
    g_MenuAltLayout = 0;
    DrawOwnedCarCounter(0, 9);
    CHECK(g_OwnedCarCounterSlide == 0 && s_spriteCount == 0);

    g_OwnedCarCounterSlide = 11;
    DrawOwnedCarCounter(1, 9);
    CHECK(g_OwnedCarCounterSlide == 12);
    CHECK(s_numberCount == 2 && s_firstNumber == 9 && s_numberY == 0x21B);
    CHECK(s_spriteCount == 2 && s_spriteY == 0x21B);
    CHECK(s_buttonCount == 1 && s_buttonY == 0x211);

    ResetDraws();
    g_OwnedCarCounterSlide = 25;
    DrawOwnedCarCounter(1, 13);
    CHECK(g_OwnedCarCounterSlide == 25);
    CHECK(s_numberY == 0xBD && s_buttonY == 0xB3);

    ResetDraws();
    g_MenuAltLayout = 1;
    DrawOwnedCarCounter(-1, 13);
    CHECK(g_OwnedCarCounterSlide == 24 && s_numberCount == 0);

    g_OwnedCarCounterSlide = INT_MAX;
    DrawOwnedCarCounter(INT_MAX, 13);
    CHECK(g_OwnedCarCounterSlide == 25);
    g_OwnedCarCounterSlide = INT_MIN;
    DrawOwnedCarCounter(-1, 13);
    CHECK(g_OwnedCarCounterSlide == 0);

    ResetDraws();
    g_MenuAltLayout = 0;
    g_OwnedCarCounterSlide = 11;
    DrawOwnedCarCounter(1, -10);
    CHECK(s_firstNumber == 0);
    ResetDraws();
    g_OwnedCarCounterSlide = 11;
    DrawOwnedCarCounter(1, INT_MAX);
    CHECK(s_firstNumber == GAME_CAR_COUNT);

    ResetDraws();
    g_RenderState.primData = NULL;
    g_OwnedCarCounterSlide = 11;
    DrawOwnedCarCounter(1, 9);
    CHECK(g_OwnedCarCounterSlide == 12);
    CHECK(s_numberCount == 0 && s_spriteCount == 0 && s_buttonCount == 0);

    puts("paint palette and owned car counter preserve their animations");
    return 0;
}
