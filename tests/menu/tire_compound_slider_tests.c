#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>

s32 g_AnimTimer;
s32 g_TireSliderPulsePhase;
GameRenderState g_RenderState;

static s32 s_directionTriangles;
static s32 s_highlight;
static s32 s_lineCount;
static s32 s_rsinAngle;
static s32 s_rsinValue;

s32 rsin(s32 angle) {
    s_rsinAngle = angle;
    return s_rsinValue;
}

void DrawLine(GameOrderingTableEntry *ot, s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g,
              s32 b, s32 alpha) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)alpha;
    s_lineCount++;
    if (r == 0 && b == 0) {
        s_highlight = g;
    }
}

void DrawFlatTriangle(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)g;
    (void)b;
    (void)semiTrans;
    (void)flags;
    if (r == 0) {
        s_directionTriangles++;
    }
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)textureU;
    (void)textureV;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
}

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height, u8 r,
                     u8 g, u8 b, u8 code) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)code;
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
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
    s_directionTriangles = 0;
    s_highlight = -1;
    s_lineCount = 0;
}

int main(void) {
    static GameOrderingTableEntry orderingTable[3];

    RENDER_OT_BASE = orderingTable;
    g_AnimTimer = 2;
    DrawTireCompoundSlider(4, 1);
    CHECK(s_directionTriangles == 1 && s_lineCount == 7);
    CHECK(s_highlight == 0xFF && g_TireSliderPulsePhase == 0x60);

    ResetDraws();
    g_AnimTimer = 0;
    DrawTireCompoundSlider(0, 1);
    CHECK(s_directionTriangles == 1 && s_lineCount == 7);
    CHECK(s_highlight == 0x60);

    ResetDraws();
    DrawTireCompoundSlider(2, 1);
    CHECK(s_directionTriangles == 2 && s_lineCount == 11);

    ResetDraws();
    g_TireSliderPulsePhase = 17;
    s_rsinValue = -4000;
    DrawTireCompoundSlider(2, 0);
    CHECK(s_rsinAngle == 17);
    CHECK(s_highlight == (u8)(-4000 / 64 - 0x41));
    CHECK(g_TireSliderPulsePhase == 17 + 0x60);

    ResetDraws();
    g_TireSliderPulsePhase = -1;
    DrawTireCompoundSlider(UINT8_MAX, 0);
    CHECK(s_rsinAngle == 0xFFF);
    CHECK(s_directionTriangles == 1 && s_lineCount == 7);

    g_TireSliderPulsePhase = INT_MAX;
    DrawTireCompoundSlider(2, 1);
    CHECK(g_TireSliderPulsePhase ==
          (s32)((u32)INT_MAX + 0x60u));

    ResetDraws();
    RENDER_OT_BASE = NULL;
    g_TireSliderPulsePhase = 12;
    DrawTireCompoundSlider(2, 0);
    CHECK(s_lineCount == 0 && s_directionTriangles == 0);
    CHECK(g_TireSliderPulsePhase == 12);

    puts("tire compound slider tests passed");
    return 0;
}
