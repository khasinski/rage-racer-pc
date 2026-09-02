#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>

s32 g_ClassChangeCurtainSlide;
s32 g_MenuAltLayout;
GameRenderState g_RenderState;

typedef struct SolidRectCall {
    GameOrderingTableEntry *ot;
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    s32 red;
    s32 green;
    s32 blue;
    s32 alpha;
} SolidRectCall;

static SolidRectCall s_rects[2];
static s32 s_rectCount;

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 red,
                   s32 green, s32 blue, s32 alpha) {
    SolidRectCall *rect = &s_rects[s_rectCount++];
    rect->ot = ot;
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
    rect->red = red;
    rect->green = green;
    rect->blue = blue;
    rect->alpha = alpha;
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
    s_rectCount = 0;
}

static int CheckPanel(s32 index, s32 y) {
    SolidRectCall *rect = &s_rects[index];
    CHECK(rect->ot == RENDER_OT_BASE_AS(void));
    CHECK(rect->x == 0 && rect->y == y);
    CHECK(rect->width == 320 && rect->height == 240);
    CHECK(rect->red == 0x95 && rect->green == 0x25 && rect->blue == 0x1E);
    CHECK(rect->alpha == 0xFF);
    return 0;
}

int main(void) {
    g_RenderState.primData = (void *)0x1234;

    g_ClassChangeCurtainSlide = 9;
    CHECK(DrawClassChangeCurtain(0) == 0);
    CHECK(s_rectCount == 0);

    CHECK(DrawClassChangeCurtain(1) == 1);
    CHECK(s_rectCount == 2);
    CHECK(CheckPanel(0, -240) == 0);
    CHECK(CheckPanel(1, 480) == 0);

    ResetDraws();
    g_ClassChangeCurtainSlide = 15;
    CHECK(DrawClassChangeCurtain(1) == 16);
    CHECK(CheckPanel(0, 0) == 0);
    CHECK(CheckPanel(1, 240) == 0);

    ResetDraws();
    g_ClassChangeCurtainSlide = 25;
    CHECK(DrawClassChangeCurtain(INT_MAX) == 25);
    CHECK(CheckPanel(0, 0) == 0);
    CHECK(CheckPanel(1, 240) == 0);

    ResetDraws();
    CHECK(DrawClassChangeCurtain(-10) == 15);
    CHECK(CheckPanel(0, 0) == 0);
    CHECK(CheckPanel(1, 240) == 0);

    ResetDraws();
    CHECK(DrawClassChangeCurtain(INT_MIN) == 0);
    CHECK(CheckPanel(0, -240) == 0);
    CHECK(CheckPanel(1, 480) == 0);

    ResetDraws();
    g_MenuAltLayout = 1;
    CHECK(DrawClassChangeCurtain(3) == 3);
    CHECK(s_rectCount == 0);

    puts("class change curtain tests passed");
    return 0;
}
