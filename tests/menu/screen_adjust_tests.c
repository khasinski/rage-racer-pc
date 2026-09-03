#include "game/menu.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>

s32 g_GameMode;
s32 g_ScreenOffsetEditX;
s32 g_ScreenOffsetEditY;
u16 g_DispEnv0ScreenX;
u16 g_DispEnv0ScreenY;
u16 g_DispEnv1ScreenX;
u16 g_DispEnv1ScreenY;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
ScreenOffset g_ScreenOffsetX;
ScreenOffset g_ScreenOffsetY;

typedef struct SpriteCall {
    GameOrderingTableEntry *ot;
    s32 x, y, u, v;
} SpriteCall;

static GameFrameContext s_frame;
static u8 s_packets[16];
static SpriteCall s_calls[4];
static s32 s_callCount;
static s32 s_hintVariant;
static s32 s_lastCue;

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    (void)width;
    (void)height;
    (void)clut;
    s_calls[s_callCount++] = (SpriteCall){ot, x, y, u, v};
    return prim + 1;
}

void DrawOptionHintBar(s32 variant) { s_hintVariant = variant; }
void PlaySoundCue(s32 cue) { s_lastCue = cue; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_GameMode = OPTION_MODE_SCREEN_ADJUST;
    g_ScreenOffsetEditX = 0;
    g_ScreenOffsetEditY = 0;
    g_ScreenOffsetX.value = 5;
    g_ScreenOffsetY.value = 6;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    s_callCount = 0;
    s_hintVariant = -1;
    s_lastCue = 0;
}

int main(void) {
    Reset();
    UpdateScreenAdjustScreen();
    CHECK(s_callCount == 4 && s_hintVariant == 3);
    CHECK(s_calls[0].ot == GamePrimaryOrderingTable(51));
    CHECK(s_calls[0].x == 0x9A && s_calls[0].y == 0x88);
    CHECK(s_calls[0].u == 0xC8 && s_calls[3].u == 0xEC);
    CHECK(g_RenderState.packetCursor == s_packets + 4);

    Reset();
    g_PadPressedRepeat = PAD_UP | PAD_LEFT;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == -1 && g_ScreenOffsetEditY == -1);
    CHECK(s_lastCue == 1);
    CHECK(g_DispEnv0ScreenX == (u16)-1 && g_DispEnv0ScreenY == 28);
    CHECK(g_DispEnv1ScreenX == (u16)-1 && g_DispEnv1ScreenY == 28);

    Reset();
    g_ScreenOffsetEditX = -11;
    g_ScreenOffsetEditY = -32;
    g_PadPressedRepeat = PAD_UP | PAD_LEFT;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == -11 && g_ScreenOffsetEditY == -32);
    CHECK(s_lastCue == 0);

    Reset();
    g_ScreenOffsetEditX = 12;
    g_ScreenOffsetEditY = 13;
    g_PadPressed = PAD_CONFIRM;
    UpdateScreenAdjustScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_ScreenOffsetX.value == 12);
    CHECK(g_ScreenOffsetY.value == 13 && s_lastCue == 2);

    Reset();
    g_ScreenOffsetEditX = 12;
    g_ScreenOffsetEditY = 13;
    g_PadPressed = PAD_CANCEL;
    UpdateScreenAdjustScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_ScreenOffsetEditX == 5);
    CHECK(g_ScreenOffsetEditY == 6 && s_lastCue == 3);

    Reset();
    g_ScreenOffsetEditX = -11;
    g_PadPressedRepeat = PAD_LEFT | PAD_RIGHT;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == -11 && s_lastCue == 0);

    Reset();
    g_ScreenOffsetEditX = 12;
    g_ScreenOffsetEditY = 13;
    g_PadPressed = PAD_CANCEL;
    g_PadPressedRepeat = PAD_RIGHT | PAD_DOWN;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == 5 && g_ScreenOffsetEditY == 6);
    CHECK(s_lastCue == 3);

    Reset();
    g_ScreenOffsetEditX = INT_MIN;
    g_ScreenOffsetEditY = INT_MAX;
    g_ScreenOffsetX.value = INT_MAX;
    g_ScreenOffsetY.value = INT_MIN;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == -11 && g_ScreenOffsetEditY == 23);
    CHECK(g_ScreenOffsetX.value == 32 && g_ScreenOffsetY.value == -32);
    CHECK(g_DispEnv0ScreenX == (u16)-11 && g_DispEnv0ScreenY == 52);

    Reset();
    g_ScreenOffsetEditX = INT_MAX;
    g_ScreenOffsetEditY = INT_MIN;
    g_PadPressed = PAD_CONFIRM;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetX.value == 32 && g_ScreenOffsetY.value == -32);

    Reset();
    g_ScreenOffsetX.value = INT_MIN;
    g_ScreenOffsetY.value = INT_MAX;
    g_PadPressed = PAD_CANCEL;
    UpdateScreenAdjustScreen();
    CHECK(g_ScreenOffsetEditX == -11 && g_ScreenOffsetEditY == 23);

    puts("screen adjustment tests passed");
    return 0;
}
