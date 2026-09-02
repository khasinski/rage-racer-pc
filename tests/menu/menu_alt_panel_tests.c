#include "game/menu.h"
#include "game/render.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>

s32 g_MenuAltLayout;
s32 g_MenuAltPanelProgressA;
s32 g_MenuAltPanelProgressB;
GameRenderState g_RenderState;

typedef struct QuadCall {
    GameOrderingTableEntry *ot;
    s16 x[4];
    u16 y[4];
    u8 u[4];
    u8 v[4];
    u16 clut;
} QuadCall;

static QuadCall s_calls[2];
static s32 s_callCount;

void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1,
                          u8 v1, u8 u2, u8 v2, u8 u3, u8 v3, u8 red,
                          u8 green, u8 blue, u16 clut, s32 shade,
                          s32 semiTrans, u16 tpage) {
    QuadCall *call = &s_calls[s_callCount++];
    (void)red;
    (void)green;
    (void)blue;
    (void)shade;
    (void)semiTrans;
    (void)tpage;
    call->ot = ot;
    call->x[0] = x0;
    call->x[1] = x1;
    call->x[2] = x2;
    call->x[3] = x3;
    call->y[0] = (u16)y0;
    call->y[1] = y1;
    call->y[2] = y2;
    call->y[3] = y3;
    call->u[0] = u0;
    call->u[1] = u1;
    call->u[2] = u2;
    call->u[3] = u3;
    call->v[0] = v0;
    call->v[1] = v1;
    call->v[2] = v2;
    call->v[3] = v3;
    call->clut = clut;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    s_callCount = 0;
}

int main(void) {
    g_RenderState.primData = (void *)0x1234;

    g_MenuAltPanelProgressA = 7;
    g_MenuAltPanelProgressB = 8;
    DrawMenuAltPanel(0, 0);
    CHECK(g_MenuAltPanelProgressA == 0 && g_MenuAltPanelProgressB == 0);
    CHECK(s_callCount == 0);

    DrawMenuAltPanel(1, 0);
    CHECK(g_MenuAltPanelProgressA == 1 && s_callCount == 0);
    DrawMenuAltPanel(1, 0);
    CHECK(g_MenuAltPanelProgressA == 2 && s_callCount == 1);
    CHECK(s_calls[0].ot == RENDER_OT_BASE_AS(void));
    CHECK(s_calls[0].x[0] == 0xA8 && s_calls[0].x[1] == 0xC4);
    CHECK(s_calls[0].y[0] == 0x9E && s_calls[0].y[2] == 0x9F);
    CHECK(s_calls[0].u[0] == 0xB0 && s_calls[0].u[1] == 0xCC);
    CHECK(s_calls[0].v[0] == 0x38 && s_calls[0].v[2] == 0x6C);
    CHECK(s_calls[0].clut == 0x232);

    ResetCalls();
    DrawMenuAltPanel(-1, 0);
    CHECK(g_MenuAltPanelProgressA == 1 && s_callCount == 1);
    CHECK(s_calls[0].y[0] == 0x9E && s_calls[0].y[2] == 0x9F);

    ResetCalls();
    DrawMenuAltPanel(0, 0);
    DrawMenuAltPanel(0, 1);
    CHECK(g_MenuAltPanelProgressB == 1 && s_callCount == 0);
    DrawMenuAltPanel(0, 1);
    CHECK(g_MenuAltPanelProgressB == 2 && s_callCount == 1);
    CHECK(s_calls[0].x[0] == 0xC0 && s_calls[0].x[1] == 0x10E);
    CHECK(s_calls[0].y[0] == 0x128 && s_calls[0].y[2] == 0x129);
    CHECK(s_calls[0].u[0] == 0x61 && s_calls[0].u[1] == 0xAF);
    CHECK(s_calls[0].clut == 0x259);

    ResetCalls();
    g_MenuAltLayout = 1;
    g_MenuAltPanelProgressA = 1;
    g_MenuAltPanelProgressB = 1;
    DrawMenuAltPanel(INT_MAX, INT_MAX);
    CHECK(s_callCount == 2);
    CHECK(s_calls[0].x[0] == 0x69 && s_calls[1].x[0] == 0x92);
    CHECK(g_MenuAltPanelProgressA == 14);
    CHECK(g_MenuAltPanelProgressB == 16);

    ResetCalls();
    DrawMenuAltPanel(INT_MIN, INT_MIN);
    CHECK(g_MenuAltPanelProgressA == 0 && g_MenuAltPanelProgressB == 0);
    CHECK(s_callCount == 0);

    puts("menu alternate panel tests passed");
    return 0;
}
