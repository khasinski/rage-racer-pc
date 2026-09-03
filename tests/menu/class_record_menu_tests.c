#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/prim.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_ClassRecordMenuCursor;
DVec g_ClassRecordCellPoints[CLASS_RECORD_COUNT];
ClassRecordSprite g_ClassRecordCellSprites[CLASS_RECORD_COUNT];
Rgb g_ClassRecordNameSprites[CLASS_RECORD_COUNT + 1];
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];
GameFrameContext *g_DrawBuffer;
s32 g_GameMode;
u16 g_PadPressed;
GameRenderState g_RenderState;
s32 g_ScreenOffsetEditX;
s32 g_ScreenOffsetEditY;

static GameFrameContext s_frame;
static u8 s_packets[512];
static s32 s_lastCue;
static s32 s_soundCalls;

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                    s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    return prim + 1;
}

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                         s32 width, s32 height, s32 u, s32 v, s32 clut) {
    return GameQueueSprite(ot, prim, x, y, width, height, u, v, clut);
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 texturePage) {
    (void)ot;
    (void)texturePage;
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w,
                s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)g;
    (void)b;
    return prim + 1;
}

void DrawMenuCursorArrow(s32 x, s32 y) {
    (void)x;
    (void)y;
}

void DrawOptionHintBar(s32 variant) { (void)variant; }

void PlaySoundCue(s32 cue) {
    s_lastCue = cue;
    s_soundCalls++;
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
    s32 i;

    memset(&s_frame, 0, sizeof(s_frame));
    memset(g_ClassRecords, 0, sizeof(g_ClassRecords));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_GameMode = OPTION_MODE_CLASS_MENU;
    g_ClassRecordMenuCursor = 0;
    g_ScreenOffsetEditX = 0;
    g_ScreenOffsetEditY = 0;
    g_PadPressed = 0;
    s_lastCue = 0;
    s_soundCalls = 0;
    for (i = 0; i < CLASS_RECORD_COUNT; i++) {
        g_ClassRecordCellPoints[i].vx = (s16)i;
        g_ClassRecordCellPoints[i].vy = (s16)i;
    }
}

static int TestMenuNavigation(void) {
    Reset();
    g_PadPressed = PAD_DOWN;
    UpdateClassRecordMenu();
    CHECK(g_ClassRecordMenuCursor == 1 && s_lastCue == 1);

    Reset();
    g_PadPressed = PAD_UP;
    UpdateClassRecordMenu();
    CHECK(g_ClassRecordMenuCursor == 1 && s_lastCue == 1);

    Reset();
    g_PadPressed = PAD_CONFIRM;
    UpdateClassRecordMenu();
    CHECK(g_GameMode == OPTION_MODE_CLASS_BROWSE && s_lastCue == 2);

    Reset();
    g_ClassRecordMenuCursor = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateClassRecordMenu();
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_lastCue == 2);

    Reset();
    g_PadPressed = PAD_CANCEL;
    UpdateClassRecordMenu();
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_lastCue == 3);
    return 0;
}

static int CheckBrowseMove(s32 x, s32 y, u16 buttons, s32 expectedX,
                           s32 expectedY, s32 expectedSoundCalls) {
    Reset();
    g_GameMode = OPTION_MODE_CLASS_BROWSE;
    g_ScreenOffsetEditX = x;
    g_ScreenOffsetEditY = y;
    g_PadPressed = buttons;
    UpdateClassRecordBrowse();
    CHECK(g_ScreenOffsetEditX == expectedX);
    CHECK(g_ScreenOffsetEditY == expectedY);
    CHECK(s_soundCalls == expectedSoundCalls);
    return 0;
}

static int TestGridNavigation(void) {
    CHECK(CheckBrowseMove(0, 0, PAD_LEFT, 5, 0, 1) == 0);
    CHECK(CheckBrowseMove(5, 0, PAD_RIGHT, 0, 0, 1) == 0);
    CHECK(CheckBrowseMove(2, 0, PAD_DOWN, 2, 1, 1) == 0);
    CHECK(CheckBrowseMove(2, 1, PAD_UP, 2, 0, 1) == 0);
    CHECK(CheckBrowseMove(4, 1, PAD_RIGHT, 5, 0, 1) == 0);
    CHECK(CheckBrowseMove(0, 1, PAD_LEFT, 5, 0, 1) == 0);
    CHECK(CheckBrowseMove(5, 0, PAD_DOWN, 5, 0, 0) == 0);

    Reset();
    g_GameMode = OPTION_MODE_CLASS_BROWSE;
    g_PadPressed = PAD_CANCEL;
    UpdateClassRecordBrowse();
    CHECK(g_GameMode == OPTION_MODE_CLASS_MENU && s_lastCue == 2);

    CHECK(CheckBrowseMove(INT_MIN, INT_MAX, 0, 0, 1, 0) == 0);
    CHECK(CheckBrowseMove(INT_MAX, INT_MIN, 0, 5, 0, 0) == 0);
    return 0;
}

static int TestInvalidMenuCursor(void) {
    Reset();
    g_ClassRecordMenuCursor = INT_MIN;
    UpdateClassRecordMenu();
    CHECK(g_ClassRecordMenuCursor == 0 && s_soundCalls == 0);

    Reset();
    g_ClassRecordMenuCursor = INT_MAX;
    UpdateClassRecordMenu();
    CHECK(g_ClassRecordMenuCursor == 1 && s_soundCalls == 0);
    return 0;
}

int main(void) {
    CHECK(TestMenuNavigation() == 0);
    CHECK(TestGridNavigation() == 0);
    CHECK(TestInvalidMenuCursor() == 0);
    puts("class record menu tests passed");
    return 0;
}
