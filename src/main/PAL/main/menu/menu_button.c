#include "game/menu.h"

void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height,
                        u8 r, u8 g, u8 b) {
    void *ot = RENDER_OT_BASE;

    DrawRectOutline(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                    0xB4, 0xB4, 0xB4, 0xFF);
    DrawSolidRect(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                  r, g, b, 0xFF);
}

void DrawMenuCursorBox(s32 x, s32 y, s32 width, s32 height, s32 useFlash) {
    void *ot = RENDER_OT_BASE;
    s32 colour;

    if (useFlash) {
        colour = g_AnimTimer & 2 ? 0xFF : 0x60;
    } else {
        colour = (rsin(g_MenuCursorPulsePhase % 4096) / 64) - 0x41;
    }

    DrawRectOutline(ot, (s16)(x - 1), (s16)(y - 2),
                    (s16)(width + 2), (s16)(height + 4),
                    0, (u8)colour, 0, 0xFF);
    DrawRectOutline(ot, (s16)x, (s16)y, (s16)width, (s16)height,
                    0, (u8)colour, 0, 0xFF);
    g_MenuCursorPulsePhase += 0x60;
}
