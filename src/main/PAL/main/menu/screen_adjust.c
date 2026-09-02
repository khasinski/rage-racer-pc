#include "game/audio.h"
#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    OPTION_MODE_ROOT = 1,
    SCREEN_OFFSET_MIN_X = -11,
    SCREEN_OFFSET_MAX_X = 32,
    SCREEN_OFFSET_MIN_Y = -32,
    SCREEN_OFFSET_MAX_Y = 23
};

void DrawScreenAdjustScreen(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);

    next = GameQueueSpriteTrans(ot, next, 0x9A, 0x88, 0xC, 0x18,
                                0xC8, 0x48, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x9A, 0xB8, 0xC, 0x18,
                                0xD4, 0x48, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0xA6, 0xA0, 0xC, 0x18,
                                0xE0, 0x48, 0x7F40);
    RENDER_PRIM_CURSOR_AS(u8) = GameQueueSpriteTrans(
        ot, next, 0x8E, 0xA0, 0xC, 0x18, 0xEC, 0x48, 0x7F40);
    DrawOptionHintBar(3);
}

/* g_GameModeHandlers[6]: edits the display offset, then commits or restores it. */
void UpdateScreenAdjustScreen(void) {
    s32 previousX = g_ScreenOffsetEditX;
    s32 previousY = g_ScreenOffsetEditY;

    DrawScreenAdjustScreen();

    if ((g_PadPressedRepeat & PAD_UP) &&
        g_ScreenOffsetEditY > SCREEN_OFFSET_MIN_Y) {
        g_ScreenOffsetEditY--;
    }
    if ((g_PadPressedRepeat & PAD_DOWN) &&
        g_ScreenOffsetEditY < SCREEN_OFFSET_MAX_Y) {
        g_ScreenOffsetEditY++;
    }
    if ((g_PadPressedRepeat & PAD_LEFT) &&
        g_ScreenOffsetEditX > SCREEN_OFFSET_MIN_X) {
        g_ScreenOffsetEditX--;
    }
    if ((g_PadPressedRepeat & PAD_RIGHT) &&
        g_ScreenOffsetEditX < SCREEN_OFFSET_MAX_X) {
        g_ScreenOffsetEditX++;
    }

    if (previousX != g_ScreenOffsetEditX || previousY != g_ScreenOffsetEditY) {
        PlaySoundCue(1);
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_ROOT;
        g_ScreenOffsetX.value = g_ScreenOffsetEditX;
        g_ScreenOffsetY.value = g_ScreenOffsetEditY;
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
        g_ScreenOffsetEditX = g_ScreenOffsetX.value;
        g_ScreenOffsetEditY = g_ScreenOffsetY.value;
    }

    g_DispEnv0ScreenX = g_ScreenOffsetEditX;
    g_DispEnv0ScreenY = g_ScreenOffsetEditY + 29;
    g_DispEnv1ScreenX = g_ScreenOffsetEditX;
    g_DispEnv1ScreenY = g_ScreenOffsetEditY + 29;
}
