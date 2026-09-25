#include "game/prim.h"
#include "game/race.h"
#include "game/race_scene_internal.h"
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
s32 g_SceneTimer;

static s32 s_spriteCount;
static s32 s_tileCount;
static s32 s_translucentTileCount;
static s32 s_drawAreaCount;
static s32 s_textCount;
static s32 s_drawModeCount;
static s32 s_selectionY[4];
static s32 s_retryDigitU;
static u8 *s_drawModePacket;
static const char *s_text[8];
static char s_textCopy[8][32];
static s32 s_textX[8];
static s32 s_textY[8];
static int s_modernEnabled;

int PortModernRendererEnabled(void) { return s_modernEnabled; }

void PortForceFeedbackLabel(char *text, size_t size) {
    if (text == NULL || size == 0) return;
    snprintf(text, size, "FFB OFF");
}

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
    (void)clut;
    if (s_textCount < 8) {
        snprintf(s_textCopy[s_textCount], sizeof(s_textCopy[s_textCount]),
                 "%s", text);
        s_text[s_textCount] = s_textCopy[s_textCount];
        s_textX[s_textCount] = x;
        s_textY[s_textCount] = y;
    }
    s_textCount++;
    g_RenderState.draw.packetCursor =
        (DrawPacket *)g_RenderState.draw.packetCursor + 1;
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
    if (x == 0xB8 && y == 0x8A) s_retryDigitU = u;
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
    s_text[0] = NULL;
    s_text[1] = NULL;
    g_RenderState.draw.packetCursor = s_frame.layout.primitiveBuffer;
    ResetRaceOptionMenuAnimation();
}

static int CheckLayout(s32 grandPrix, s32 expectedSprites) {
    CourseProgressState progress = {0};
    POLY_FT4 *pulse;
    s32 expectedSelectionY = grandPrix != 0 ? 0x80 : 0x88;

    Reset();
    progress.retriesRemaining = 2;
    g_CourseProgress = &progress;
    g_GrandPrixMode = grandPrix;
    DrawRaceOptionMenu(LastRacePauseOption((s16)grandPrix));

    CHECK(s_spriteCount == expectedSprites);
    CHECK(s_tileCount == 4 && s_translucentTileCount == 2);
    CHECK(s_drawAreaCount == 2 && s_textCount == 4 && s_drawModeCount == 1);
    CHECK(strcmp(s_text[0], "  RAGE RACER GE") == 0);
    CHECK(strcmp(s_text[1], "TS YOU GOING!  ") == 0);
    CHECK(strcmp(s_text[2], "CLASSIC") == 0);
    CHECK(strcmp(s_text[3], "FFB OFF") == 0);
    CHECK(s_selectionY[0] == expectedSelectionY);
    CHECK(s_selectionY[1] == expectedSelectionY + 9);
    CHECK(s_selectionY[2] == expectedSelectionY);
    CHECK(s_selectionY[3] == expectedSelectionY);
    CHECK(s_textX[0] == 99 && s_textX[1] == 219);
    CHECK(s_textX[3] == 0x84);
    CHECK(s_textY[0] == 0x92 && s_textY[1] == 0x92);
    CHECK(s_textY[2] == (grandPrix != 0 ? 0x7A : 0x82));
    CHECK(s_textY[3] == (grandPrix != 0 ? 0x82 : 0x8A));

    pulse = (POLY_FT4 *)s_drawModePacket - 1;
    CHECK(pulse->x0 == 0x74 && pulse->x1 == 0xCC);
    CHECK(pulse->y0 == 0x58 && pulse->y2 == 0x9C);
    CHECK(pulse->clut == 0x784B && pulse->tpage == 9);
    CHECK(g_RenderState.draw.packetCursor == (DrawPacket *)s_drawModePacket + 1);
    return 0;
}

int main(void) {
    if (CheckLayout(1, 6) || CheckLayout(0, 3)) return 1;

    Reset();
    s_modernEnabled = 1;
    g_GrandPrixMode = 1;
    DrawRaceOptionMenu(2);
    CHECK(strcmp(s_text[2], "MODERN") == 0);
    s_modernEnabled = 0;

    Reset();
    g_CourseProgress = NULL;
    g_GrandPrixMode = 1;
    DrawRaceOptionMenu(INT_MAX);
    CHECK(s_selectionY[0] == 0x80 && s_retryDigitU == 0);

    Reset();
    g_CourseProgress = NULL;
    g_GrandPrixMode = 0;
    DrawRaceOptionMenu(INT_MIN);
    CHECK(s_selectionY[0] == 0x68);

    puts("race option menu emits both layouts with bounded packet cursors");
    return 0;
}
