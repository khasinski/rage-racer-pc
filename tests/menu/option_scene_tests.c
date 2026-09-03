#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_AnimTimer;
s32 g_GameMode;
s32 g_OptionLetterboxHeight;
s32 g_SceneTimer;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

static GameFrameContext s_frame;
static u8 s_packets[32];
static s32 s_displayMask;
static s32 s_handlerCalls[OPTION_MODE_COUNT];
static s32 s_hintCalls;
static s32 s_lineCalls;
static s32 s_tileCalls;
static s32 s_lastTileHeight;

#define DEFINE_HANDLER(index)                                                  \
    static void Handler##index(void) { s_handlerCalls[index]++; }

DEFINE_HANDLER(0)
DEFINE_HANDLER(1)
DEFINE_HANDLER(2)
DEFINE_HANDLER(3)
DEFINE_HANDLER(4)
DEFINE_HANDLER(5)
DEFINE_HANDLER(6)
DEFINE_HANDLER(7)
DEFINE_HANDLER(8)
DEFINE_HANDLER(9)
DEFINE_HANDLER(10)
DEFINE_HANDLER(11)

void (*g_NativeGameModeHandlers[OPTION_MODE_COUNT])(void) = {
    Handler0, Handler1, Handler2, Handler3, Handler4, Handler5,
    Handler6, Handler7, Handler8, Handler9, Handler10, Handler11,
};

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w,
                s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)r;
    (void)g;
    (void)b;
    s_tileCalls++;
    s_lastTileHeight = h;
    return prim + 1;
}

u8 *GameQueueLine(GameOrderingTableEntry *ot, u8 *prim, s32 x0, s32 y0,
                  s32 x1, s32 y1, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)r;
    (void)g;
    (void)b;
    s_lineCalls++;
    return prim + 1;
}

void DrawPadTypeHint(void) { s_hintCalls++; }
void SetDispMask(s32 enabled) { s_displayMask = enabled; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(OptionMode mode) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(s_handlerCalls, 0, sizeof(s_handlerCalls));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_AnimTimer = 10;
    g_GameMode = mode;
    g_OptionLetterboxHeight = 0;
    g_SceneTimer = 0;
    s_displayMask = 0;
    s_hintCalls = 0;
    s_lineCalls = 0;
    s_tileCalls = 0;
    s_lastTileHeight = -1;
}

int main(void) {
    Reset(OPTION_MODE_SOUND_EDIT);
    UpdateOptionScene();
    CHECK(s_handlerCalls[OPTION_MODE_SOUND_EDIT] == 1);
    CHECK(g_AnimTimer == 11 && g_SceneTimer == 1);
    CHECK(s_displayMask == 0 && s_hintCalls == 1);
    CHECK(g_OptionLetterboxHeight == 4);
    CHECK(s_tileCalls == 2 && s_lineCalls == 0);

    UpdateOptionScene();
    CHECK(s_handlerCalls[OPTION_MODE_SOUND_EDIT] == 2);
    CHECK(s_displayMask == 1 && g_SceneTimer == 2);

    Reset(OPTION_MODE_SCREEN_ADJUST);
    UpdateOptionScene();
    CHECK(s_handlerCalls[OPTION_MODE_SCREEN_ADJUST] == 1);
    CHECK(s_hintCalls == 1 && s_tileCalls == 4 && s_lineCalls == 2);

    Reset(OPTION_MODE_NEGCON_NEUTRAL);
    UpdateOptionScene();
    CHECK(s_handlerCalls[OPTION_MODE_NEGCON_NEUTRAL] == 1);
    CHECK(s_hintCalls == 0);

    Reset(OPTION_MODE_ROOT);
    g_OptionLetterboxHeight = 239;
    DrawOptionSceneOverlay();
    CHECK(g_OptionLetterboxHeight == 240 && s_lastTileHeight == 240);

    Reset(OPTION_MODE_ROOT);
    g_OptionLetterboxHeight = 241;
    DrawOptionSceneOverlay();
    CHECK(g_OptionLetterboxHeight == 240 && s_lastTileHeight == 240);

    Reset(OPTION_MODE_SCREEN_ADJUST);
    g_OptionLetterboxHeight = 479;
    DrawOptionSceneOverlay();
    CHECK(g_OptionLetterboxHeight == 480 && s_lastTileHeight == 480);

    Reset(OPTION_MODE_FADE);
    g_GameMode = OPTION_MODE_COUNT;
    UpdateOptionScene();
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_handlerCalls[OPTION_MODE_ROOT] == 1);

    Reset(OPTION_MODE_FADE);
    g_GameMode = -1;
    UpdateOptionScene();
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_handlerCalls[OPTION_MODE_ROOT] == 1);

    puts("option scene tests passed");
    return 0;
}
