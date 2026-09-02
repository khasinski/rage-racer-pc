#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>

GameRenderState g_RenderState;
s32 g_AnimTimer;
s32 g_MenuCursorPulsePhase;

typedef struct RectCall {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    s32 r;
    s32 g;
    s32 b;
    s32 alpha;
} RectCall;

static RectCall s_outlines[6];
static RectCall s_fills[2];
static s32 s_outlineCount;
static s32 s_fillCount;
static s32 s_failures;

static void Check(s32 actual, s32 expected, const char *label) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", label, actual, expected);
        s_failures++;
    }
}

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height,
                     u8 r, u8 g, u8 b, u8 alpha) {
    RectCall *call = &s_outlines[s_outlineCount++];
    (void)ot;
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
    call->r = r;
    call->g = g;
    call->b = b;
    call->alpha = alpha;
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height,
                   s32 r, s32 g, s32 b, s32 alpha) {
    RectCall *call = &s_fills[s_fillCount++];
    (void)ot;
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
    call->r = r;
    call->g = g;
    call->b = b;
    call->alpha = alpha;
}

int main(void) {
    GameDrawMenuButton(10, 20, 30, 40, 1, 2, 3);
    Check(s_outlineCount, 1, "button outline count");
    Check(s_fillCount, 1, "button fill count");
    Check(s_outlines[0].r, 0xB4, "button border red");
    Check(s_outlines[0].alpha, 0xFF, "button border alpha");
    Check(s_fills[0].x, 10, "button fill x");
    Check(s_fills[0].height, 40, "button fill height");
    Check(s_fills[0].b, 3, "button fill blue");
    Check(s_fills[0].alpha, 0xFF, "button fill alpha");

    g_AnimTimer = 0;
    g_MenuCursorPulsePhase = 123;
    DrawMenuCursorBox(50, 60, 70, 80, 1);
    Check(s_outlineCount, 3, "cursor outline count");
    Check(s_outlines[1].x, 49, "outer cursor x");
    Check(s_outlines[1].y, 58, "outer cursor y");
    Check(s_outlines[1].width, 72, "outer cursor width");
    Check(s_outlines[1].height, 84, "outer cursor height");
    Check(s_outlines[1].g, 0x60, "cursor flash colour");
    Check(s_outlines[2].x, 50, "inner cursor x");
    Check(g_MenuCursorPulsePhase, 123 + 0x60, "cursor phase advance");

    g_AnimTimer = 2;
    DrawMenuCursorBox(0, 0, 1, 1, 1);
    Check(s_outlines[3].g, 0xFF, "bright cursor flash colour");

    if (s_failures != 0) {
        printf("%d menu button assertion(s) failed\n", s_failures);
        return 1;
    }
    puts("menu buttons emit their fill and selection outlines");
    return 0;
}
