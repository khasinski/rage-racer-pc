#include "game/menu.h"

enum {
    MENU_BUTTON_BORDER_INTENSITY = 0xB4,
    MENU_CURSOR_FLASH_DIM = 0x60,
    MENU_CURSOR_FLASH_BRIGHT = 0xFF,
    MENU_CURSOR_PULSE_BASE = 0xBF,
    MENU_CURSOR_PULSE_STEP = 0x60,
};

void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height,
                        u8 r, u8 g, u8 b) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;

    if (ot == NULL) {
        return;
    }

    DrawRectOutline(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                    MENU_BUTTON_BORDER_INTENSITY,
                    MENU_BUTTON_BORDER_INTENSITY,
                    MENU_BUTTON_BORDER_INTENSITY, 0xFF);
    DrawSolidRect(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                  r, g, b, 0xFF);
}

void DrawMenuCursorBox(s32 x, s32 y, s32 width, s32 height, s32 useFlash) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 colour;

    if (ot == NULL) {
        return;
    }

    if (useFlash != 0) {
        colour = (g_AnimTimer & 2) != 0 ? MENU_CURSOR_FLASH_BRIGHT
                                        : MENU_CURSOR_FLASH_DIM;
    } else {
        colour = MENU_CURSOR_PULSE_BASE +
                 rsin((s32)((u32)g_MenuCursorPulsePhase & 0xFFFu)) / 64;
    }

    DrawRectOutline(ot, (s16)(x - 1), (s16)(y - 2),
                    (s16)(width + 2), (s16)(height + 4),
                    0, (u8)colour, 0, 0xFF);
    DrawRectOutline(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                    0, (u8)colour, 0, 0xFF);
    g_MenuCursorPulsePhase =
        (s32)((u32)g_MenuCursorPulsePhase + MENU_CURSOR_PULSE_STEP);
}
