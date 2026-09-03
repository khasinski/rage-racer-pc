#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_GameMode;
u32 g_OptionMenuExitScene;
s32 g_SceneId;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

static GameFrameContext s_frame;
static u8 s_packets[8];
static s32 s_drawMode;
static s32 s_rootDraws;
static s32 s_tileColor;
static s32 s_tileHeight;
static s32 s_tileWidth;

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                       s32 w, s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)g;
    (void)b;
    s_tileWidth = w;
    s_tileHeight = h;
    s_tileColor = r;
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    s_drawMode = tpage;
    return prim + 1;
}

void DrawOptionRootMenu(void) { s_rootDraws++; }

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
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_GameMode = OPTION_MODE_ROOT;
    g_OptionMenuExitScene = 0;
    g_SceneId = 7;
    s_drawMode = -1;
    s_rootDraws = 0;
    s_tileColor = -1;
    s_tileHeight = 0;
    s_tileWidth = 0;
}

static int TestFadeTileClamping(void) {
    Reset();
    DrawFullscreenFadeTile480(-4, 0x49);
    CHECK(s_tileColor == 0 && s_tileWidth == 0x140);
    CHECK(s_tileHeight == 0x1E0 && s_drawMode == 0x49);
    CHECK(g_RenderState.packetCursor == s_packets + 2);

    Reset();
    DrawFullscreenFadeTile480(0x100, 3);
    CHECK(s_tileColor == 0xFF && s_drawMode == 3);
    return 0;
}

static int TestFadeStateTransitions(void) {
    Reset();
    StartOptionMenuExit(27);
    CHECK(g_OptionMenuExitScene == 27);
    CHECK(g_GameMode == OPTION_MODE_FADE && g_FadeStep == 8);

    g_FadeLevel = 0;
    g_FadeStep = -8;
    UpdateOptionMenuFade();
    CHECK(g_FadeLevel == -8 && g_FadeStep == 0);
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_SceneId == 7);
    CHECK(s_tileColor == 0 && s_rootDraws == 1);

    Reset();
    g_GameMode = OPTION_MODE_FADE;
    g_OptionMenuExitScene = 31;
    g_FadeLevel = 0x100;
    g_FadeStep = 8;
    UpdateOptionMenuFade();
    CHECK(g_FadeLevel == 0x108 && g_SceneId == 31);
    CHECK(g_GameMode == OPTION_MODE_FADE && s_tileColor == 0xFF);
    CHECK(s_rootDraws == 1);
    return 0;
}

int main(void) {
    CHECK(TestFadeTileClamping() == 0);
    CHECK(TestFadeStateTransitions() == 0);
    puts("option menu fade tests passed");
    return 0;
}
