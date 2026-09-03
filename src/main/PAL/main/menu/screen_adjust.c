#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    SCREEN_OFFSET_MIN_X = -11,
    SCREEN_OFFSET_MAX_X = 32,
    SCREEN_OFFSET_MIN_Y = -32,
    SCREEN_OFFSET_MAX_Y = 23
};

static s32 AdjustScreenOffset(s32 value, u16 buttons, u16 decreaseButton,
                              u16 increaseButton, s32 minimum, s32 maximum) {
    s32 direction = ((buttons & increaseButton) != 0) -
                    ((buttons & decreaseButton) != 0);

    return AddClampedMenuValue(value, direction, minimum, maximum);
}

static void NormalizeScreenOffsets(void) {
    g_ScreenOffsetX = AddClampedMenuValue(
        g_ScreenOffsetX, 0, SCREEN_OFFSET_MIN_X, SCREEN_OFFSET_MAX_X);
    g_ScreenOffsetY = AddClampedMenuValue(
        g_ScreenOffsetY, 0, SCREEN_OFFSET_MIN_Y, SCREEN_OFFSET_MAX_Y);
    g_ScreenOffsetEditX = AddClampedMenuValue(
        g_ScreenOffsetEditX, 0, SCREEN_OFFSET_MIN_X, SCREEN_OFFSET_MAX_X);
    g_ScreenOffsetEditY = AddClampedMenuValue(
        g_ScreenOffsetEditY, 0, SCREEN_OFFSET_MIN_Y, SCREEN_OFFSET_MAX_Y);
}

static void DrawScreenAdjustScreen(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);

    next = GameQueueSpriteTrans(ot, next, 0x9A, 0x88, 0xC, 0x18,
                                0xC8, 0x48, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x9A, 0xB8, 0xC, 0x18,
                                0xD4, 0x48, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0xA6, 0xA0, 0xC, 0x18,
                                0xE0, 0x48, 0x7F40);
    g_RenderState.packetCursor = GameQueueSpriteTrans(
        ot, next, 0x8E, 0xA0, 0xC, 0x18, 0xEC, 0x48, 0x7F40);
    DrawOptionHintBar(3);
}

/* OPTION_MODE_SCREEN_ADJUST: edits the display offset, then commits or
 * restores it. */
void UpdateScreenAdjustScreen(void) {
    s32 previousX;
    s32 previousY;

    NormalizeScreenOffsets();
    previousX = g_ScreenOffsetEditX;
    previousY = g_ScreenOffsetEditY;
    DrawScreenAdjustScreen();

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_ROOT;
        g_ScreenOffsetX = g_ScreenOffsetEditX;
        g_ScreenOffsetY = g_ScreenOffsetEditY;
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
        g_ScreenOffsetEditX = g_ScreenOffsetX;
        g_ScreenOffsetEditY = g_ScreenOffsetY;
    } else {
        g_ScreenOffsetEditX = AdjustScreenOffset(
            g_ScreenOffsetEditX, g_PadPressedRepeat, PAD_LEFT, PAD_RIGHT,
            SCREEN_OFFSET_MIN_X, SCREEN_OFFSET_MAX_X);
        g_ScreenOffsetEditY = AdjustScreenOffset(
            g_ScreenOffsetEditY, g_PadPressedRepeat, PAD_UP, PAD_DOWN,
            SCREEN_OFFSET_MIN_Y, SCREEN_OFFSET_MAX_Y);
        if (previousX != g_ScreenOffsetEditX ||
            previousY != g_ScreenOffsetEditY) {
            PlaySoundCue(1);
        }
    }

    g_DispEnv0ScreenX = g_ScreenOffsetEditX;
    g_DispEnv0ScreenY = g_ScreenOffsetEditY + 29;
    g_DispEnv1ScreenX = g_ScreenOffsetEditX;
    g_DispEnv1ScreenY = g_ScreenOffsetEditY + 29;
}
