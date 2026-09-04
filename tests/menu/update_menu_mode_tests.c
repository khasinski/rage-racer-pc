#include "game/car.h"
#include "game/menu.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_AnimTimer;
s32 g_CarListCursor;
s32 g_CarSpecGraphStep;
CarEntry *g_CarTable;
GameRenderState g_RenderState;
s32 g_MenuHandlerIndex;
s32 g_MenuHintBarProgress;
TimedDrawCommand g_MenuHintBarScript[1];
s32 g_MenuHintBarStep;
s32 g_MenuHintButtonsVisible;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOutgoingScreenProgress;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
u8 g_PadType;
s32 g_PlayerCarIndex;
s32 g_SceneTimer;

static CarEntry s_cars[3];
static GameOrderingTableEntry s_orderingTable[4];
static s32 s_displayMask;
static s32 s_drawCalls;
static s32 s_drawSteps[2];
static s32 s_hintResult;
static s32 s_overlayCalls;
static s32 s_specCarTire;
static s32 s_updateCalls;

static void CountUpdate(void) { s_updateCalls++; }
static s32 CountDraw(s32 step) {
    s_drawSteps[s_drawCalls++] = step;
    return 123;
}

void (*g_MenuScreenUpdate[MENU_SCREEN_COUNT])(void);
s32 (*g_MenuScreenDraw[MENU_SCREEN_COUNT])(s32 step);

void SetDispMask(s32 enabled) { s_displayMask = enabled; }

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width,
                   s32 height, s32 r, s32 g, s32 b, s32 depth) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)depth;
}

void DrawCarSpecGraph(s32 step, u32 tireGrade) {
    (void)step;
    s_specCarTire = (s32)tireGrade;
}

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    *progress += step;
    return s_hintResult;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1,
                u16 u0, u16 v0, u8 r, u8 g, u8 b, u16 clutX,
                s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)u0;
    (void)v0;
    (void)r;
    (void)g;
    (void)b;
    (void)clutX;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
}

void DrawBitPatternOverlay(s32 pattern) {
    (void)pattern;
    s_overlayCalls++;
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

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(s_cars, 0, sizeof(s_cars));
    for (i = 0; i < MENU_SCREEN_COUNT; i++) {
        g_MenuScreenUpdate[i] = CountUpdate;
        g_MenuScreenDraw[i] = CountDraw;
    }
    g_CarTable = s_cars;
    RENDER_OT_BASE = s_orderingTable;
    g_AnimTimer = 10;
    g_SceneTimer = 0;
    g_MenuScreen = MENU_SCREEN_COURSE_SELECT;
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = -1;
    g_MenuOutgoingScreenProgress = -1;
    g_CarSpecGraphStep = 7;
    g_PlayerCarIndex = 1;
    g_CarListCursor = 2;
    s_cars[1].tireCompound = 4;
    s_cars[2].tireCompound = 8;
    g_MenuHintBarStep = 0;
    g_MenuHintBarProgress = 0;
    g_MenuHintButtonsVisible = 0;
    g_MenuOverlayPattern = 0;
    g_PadType = PAD_TYPE_DIGITAL;
    s_displayMask = 0;
    s_drawCalls = 0;
    s_hintResult = 0;
    s_overlayCalls = 0;
    s_specCarTire = -1;
    s_updateCalls = 0;
}

static int TestDispatchAndLayers(void) {
    Reset();
    g_SceneTimer = 1;
    g_MenuHandlerIndex = MENU_SCREEN_RANKING;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_CUSTOMIZE;
    UpdateMenuMode();
    CHECK(g_AnimTimer == 11 && g_SceneTimer == 2 && s_displayMask == 1);
    CHECK(g_RenderState.otShift == 1 && s_updateCalls == 1);
    CHECK(s_drawCalls == 2 && s_drawSteps[0] == 0x14);
    CHECK(s_drawSteps[1] == -10 && g_MenuOutgoingScreenProgress == 123);
    CHECK(s_specCarTire == 4 && s_overlayCalls == 0);

    Reset();
    g_MenuScreen = MENU_SCREEN_CAR_SHOP;
    g_MenuHintBarStep = 1;
    s_hintResult = 1;
    UpdateMenuMode();
    CHECK(g_RenderState.otShift == 5 && s_specCarTire == 8);
    CHECK(g_MenuHintBarProgress == 1 && s_overlayCalls == 1);
    return 0;
}

static int TestInvalidIndices(void) {
    Reset();
    g_MenuScreen = -1;
    g_MenuHandlerIndex = MENU_SCREEN_COUNT;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_COUNT + 20;
    UpdateMenuMode();
    CHECK(g_MenuScreen == MENU_SCREEN_BOOTSTRAP);
    CHECK(s_updateCalls == 1 && s_drawCalls == 0);
    CHECK(g_RenderState.otShift == 5);

    Reset();
    g_PlayerCarIndex = -1;
    UpdateMenuMode();
    CHECK(s_specCarTire == 0);

    Reset();
    g_MenuScreen = MENU_SCREEN_CAR_SHOP;
    g_CarListCursor = GAME_CAR_COUNT;
    UpdateMenuMode();
    CHECK(s_specCarTire == 0);

    Reset();
    g_CarTable = NULL;
    UpdateMenuMode();
    CHECK(s_specCarTire == 0);
    return 0;
}

static int TestTimerWrap(void) {
    Reset();
    g_AnimTimer = INT_MAX;
    g_SceneTimer = INT_MAX;
    UpdateMenuMode();
    CHECK(g_AnimTimer == INT_MIN && g_SceneTimer == INT_MIN);
    CHECK(s_displayMask == 0);
    return 0;
}

int main(void) {
    CHECK(TestDispatchAndLayers() == 0);
    CHECK(TestInvalidIndices() == 0);
    CHECK(TestTimerWrap() == 0);
    puts("menu mode dispatcher tests passed");
    return 0;
}
