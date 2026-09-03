#include "game/prim.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render_internal.h"
#include "game/save_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
CourseProgressState *g_CourseProgress;
s16 g_GrandPrixMode;
s32 g_RaceOptionPulseAngle;
s16 g_RaceOptionScroll0;
s16 g_RaceOptionScroll1;
s32 g_SceneTimer;
char g_RaceOptionMarquee[4][40];

static s32 s_spriteCount;
static s32 s_tileCount;
static s32 s_translucentTileCount;
static s32 s_drawAreaCount;
static s32 s_textCount;
static s32 s_drawModeCount;
static s32 s_selectionY[4];
static s32 s_retryDigitU;
static u8 *s_drawModePacket;

s32 rcos(s32 angle) {
    (void)angle;
    return 4096;
}

u8 *QueueDrawAreaPrim(GameOrderingTableEntry *ot, DrawPacket *packet,
                      s16 x, s16 y, s32 width, s32 height) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    s_drawAreaCount++;
    return (u8 *)(packet + 1);
}

void DrawText8x8(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
    s_textCount++;
    g_RenderState.packetCursor =
        (DrawPacket *)g_RenderState.packetCursor + 1;
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                    s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_spriteCount++;
    if (x == 0xB8 && y == 0x7E) s_retryDigitU = u;
    return (u8 *)((SPRT *)packet + 1);
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                s32 width, s32 height, s32 red, s32 green, s32 blue) {
    (void)ot;
    (void)x;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    s_selectionY[s_tileCount++] = y;
    return (u8 *)((TILE *)packet + 1);
}

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                       s32 width, s32 height, s32 red, s32 green, s32 blue) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    s_translucentTileCount++;
    return (u8 *)((TILE *)packet + 1);
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    (void)ot;
    if (tpage != 9) return NULL;
    s_drawModeCount++;
    s_drawModePacket = packet;
    return (u8 *)((DrawPacket *)packet + 1);
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    s_spriteCount = 0;
    s_tileCount = 0;
    s_translucentTileCount = 0;
    s_drawAreaCount = 0;
    s_textCount = 0;
    s_drawModeCount = 0;
    s_drawModePacket = NULL;
    s_retryDigitU = -1;
    g_RenderState.packetCursor = s_frame.layout.primitiveBuffer;
    g_RaceOptionScroll0 = 0;
    g_RaceOptionScroll1 = 0;
    g_RaceOptionPulseAngle = -32;
}

static int CheckLayout(s32 grandPrix, s32 expectedSprites) {
    CourseProgressState progress = {0};
    POLY_FT4 *pulse;
    s32 expectedSelectionY = grandPrix != 0 ? 0x72 : 0x7C;

    Reset();
    progress.retriesRemaining = 2;
    g_CourseProgress = &progress;
    g_GrandPrixMode = grandPrix;
    DrawRaceOptionMenu(2);

    CHECK(s_spriteCount == expectedSprites);
    CHECK(s_tileCount == 4 && s_translucentTileCount == 2);
    CHECK(s_drawAreaCount == 2 && s_textCount == 2 && s_drawModeCount == 1);
    CHECK(s_selectionY[0] == expectedSelectionY);
    CHECK(s_selectionY[1] == expectedSelectionY + 0xB);
    CHECK(s_selectionY[2] == expectedSelectionY);
    CHECK(s_selectionY[3] == expectedSelectionY);
    CHECK(g_RaceOptionScroll0 == -4 && g_RaceOptionScroll1 == -4);
    CHECK(g_RaceOptionPulseAngle == 0);

    pulse = (POLY_FT4 *)s_drawModePacket - 1;
    CHECK(pulse->x0 == 0x74 && pulse->x1 == 0xCC);
    CHECK(pulse->y0 == 0x58 && pulse->y2 == 0x90);
    CHECK(pulse->clut == 0x784B && pulse->tpage == 9);
    CHECK(g_RenderState.packetCursor == (DrawPacket *)s_drawModePacket + 1);
    return 0;
}

int main(void) {
    if (CheckLayout(1, 6) || CheckLayout(0, 3)) return 1;

    Reset();
    g_CourseProgress = NULL;
    g_GrandPrixMode = 1;
    DrawRaceOptionMenu(INT_MAX);
    CHECK(s_selectionY[0] == 0x72 && s_retryDigitU == 0);

    Reset();
    g_CourseProgress = NULL;
    g_GrandPrixMode = 0;
    DrawRaceOptionMenu(INT_MIN);
    CHECK(s_selectionY[0] == 0x68);

    puts("race option menu emits both layouts with bounded packet cursors");
    return 0;
}
