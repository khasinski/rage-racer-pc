#include "common.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 g_MenuLightBurstLevel;
const MenuLightBurstBand g_MenuLightBurstBandX = {
    {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
     31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
     42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52}};
const MenuLightBurstBand g_MenuLightBurstBandY = {
    {170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
     181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
     192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202}};
GameRenderState g_RenderState;

typedef struct LineRecord {
    s32 x0;
    s32 x1;
    s32 topShade;
    s32 bottomShade;
} LineRecord;

typedef struct BandRecord {
    s32 x;
    s32 y;
    s32 width;
    s32 shade;
} BandRecord;

static LineRecord s_lines[33];
static BandRecord s_bands[33];
static s32 s_lineCount;
static s32 s_bandCount;
static s32 s_clipCount;
static s32 s_firstClipX;
static s32 s_lastClipX;

void DrawGradientLine(void *ot, s32 x0, s32 y0, s32 x1, u16 y1, u8 r0,
                      u8 g0, u8 b0, u8 r1, u8 g1, u8 b1, u8 alpha) {
    (void)ot;
    (void)y0;
    (void)y1;
    (void)g0;
    (void)b0;
    (void)g1;
    (void)b1;
    (void)alpha;
    s_lines[s_lineCount++] = (LineRecord){x0, x1, r0, r1};
}

void DrawSolidRect(void *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)height;
    (void)g;
    (void)b;
    (void)alpha;
    s_bands[s_bandCount++] = (BandRecord){x, y, width, r};
}

void SetDrawClipRect(void *ot, s32 x, s32 y, s32 width, s32 height) {
    (void)ot;
    (void)y;
    (void)width;
    (void)height;
    if (s_clipCount == 0) {
        s_firstClipX = x;
    }
    s_lastClipX = x;
    s_clipCount++;
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
    s_lineCount = 0;
    s_bandCount = 0;
    s_clipCount = 0;
}

int main(void) {
    GameOrderingTableEntry orderingTable[0x300];
    u8 packets[sizeof(POLY_G4) * 2];
    POLY_G4 *quad = (POLY_G4 *)packets;
    memset(orderingTable, 0, sizeof(orderingTable));
    memset(packets, 0, sizeof(packets));
    RENDER_OT_BASE_AS(GameOrderingTableEntry) = orderingTable;
    RENDER_PRIM_CURSOR_AS(u8) = packets;

    g_MenuLightBurstLevel = 99;
    DrawMenuLightBurst(0);
    CHECK(g_MenuLightBurstLevel == 0 && s_lineCount == 0);

    DrawMenuLightBurst(7);
    CHECK(g_MenuLightBurstLevel == 7 && s_lineCount == 0);
    DrawMenuLightBurst(7);
    CHECK(g_MenuLightBurstLevel == 14);
    CHECK(s_lineCount == 33 && s_bandCount == 33 && s_clipCount == 2);
    CHECK(s_lines[0].x0 == 0x30 && s_lines[0].x1 == 0);
    CHECK(s_lines[32].x0 == 0x110 && s_lines[32].x1 == 320);
    CHECK(s_lines[0].topShade == 0 && s_lines[0].bottomShade == 2);
    CHECK(s_bands[0].x == 20 && s_bands[0].y == 170);
    CHECK(s_bands[0].width == 280 && s_bands[0].shade == 0);
    CHECK(s_firstClipX == 0 && s_lastClipX == 0x48);
    CHECK(quad->x0 == 0 && quad->y0 == 0x28);
    CHECK(quad->x1 == 0x13F && quad->y1 == 0x28);
    CHECK(quad->x2 == 0 && quad->y2 == 0x1DF);
    CHECK(quad->x3 == 0x13F && quad->y3 == 0x1DF);
    CHECK(quad->r0 == 0 && quad->r2 == 1 && quad->r3 == 1);
    CHECK(RENDER_PRIM_CURSOR_AS(u8) == packets + sizeof(POLY_G4));

    ResetDraws();
    g_MenuLightBurstLevel = 5;
    DrawMenuLightBurst(-9);
    CHECK(g_MenuLightBurstLevel == 0 && s_lineCount == 0);

    g_MenuLightBurstLevel = 510;
    DrawMenuLightBurst(7);
    CHECK(g_MenuLightBurstLevel == 512);

    puts("menu light burst preserves its rays, bands, fade and animation");
    return 0;
}
