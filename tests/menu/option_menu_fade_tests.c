#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

#include <limits.h>
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
static u8 s_packets[64];
static s32 s_drawMode;
static s32 s_rootDraws;

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
}

static int TestFadeTileClamping(void) {
    TILE *tile;

    Reset();
    DrawFullscreenFadeTile480(-4, 0x49);
    tile = (TILE *)s_packets;
    CHECK(tile->r0 == 0 && tile->g0 == 0 && tile->b0 == 0);
    CHECK(tile->w == 320 && tile->h == 480 && s_drawMode == 0x49);
    CHECK(tile->code == 0x62);
    CHECK(g_RenderState.packetCursor == (u8 *)(tile + 1) + 1);

    Reset();
    DrawFullscreenFadeTile480(0x100, 3);
    tile = (TILE *)s_packets;
    CHECK(tile->r0 == 0xFF && tile->g0 == 0xFF && tile->b0 == 0xFF);
    CHECK(s_drawMode == 3);
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
    CHECK(g_FadeLevel == 0 && g_FadeStep == 0);
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_SceneId == 7);
    CHECK(((TILE *)s_packets)->r0 == 0 && s_rootDraws == 1);

    Reset();
    g_GameMode = OPTION_MODE_FADE;
    g_OptionMenuExitScene = 31;
    g_FadeLevel = 0x100;
    g_FadeStep = 8;
    UpdateOptionMenuFade();
    CHECK(g_FadeLevel == 0x100 && g_SceneId == 31);
    CHECK(g_GameMode == OPTION_MODE_FADE &&
          ((TILE *)s_packets)->r0 == 0xFF);
    CHECK(s_rootDraws == 1);

    Reset();
    g_GameMode = OPTION_MODE_FADE;
    g_OptionMenuExitScene = 19;
    g_FadeLevel = INT_MAX;
    g_FadeStep = INT_MAX;
    UpdateOptionMenuFade();
    CHECK(g_FadeLevel == 0x100 && g_SceneId == 19);

    Reset();
    g_GameMode = OPTION_MODE_FADE;
    g_FadeLevel = INT_MIN;
    g_FadeStep = INT_MIN;
    UpdateOptionMenuFade();
    CHECK(g_FadeLevel == 0 && g_FadeStep == 0);
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_SceneId == 7);
    return 0;
}

int main(void) {
    CHECK(TestFadeTileClamping() == 0);
    CHECK(TestFadeStateTransitions() == 0);
    puts("option menu fade tests passed");
    return 0;
}
