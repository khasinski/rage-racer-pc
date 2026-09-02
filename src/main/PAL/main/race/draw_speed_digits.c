#include "game/prim.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render_internal.h"

enum {
    SPEED_DIGIT_SPACING = 8,
    SPEED_DIGIT_TEXTURE_PAGE = 9,
};

void DrawSpeedDigits(s32 x, s32 y, s32 value) {
    u8 *prim;
    s32 screenX;
    s32 screenY;
    s32 hundreds;
    s32 tens;
    s32 ones;
    u16 color;

    hundreds = value / 100;
    screenX = g_CarSpec->tachometer.digitsX + x;
    screenY = g_CarSpec->tachometer.digitsY + y;
    color = g_HudGlyphClut;
    prim = RENDER_PRIM_CURSOR_AS(u8);

    tens = (value / 10) % 10;
    ones = value % 10;

    prim = DrawHudDigit(prim, screenX, screenY, hundreds, color);
    prim = DrawHudDigit(prim, screenX + SPEED_DIGIT_SPACING, screenY,
                        tens, color);
    prim = DrawHudDigit(prim, screenX + SPEED_DIGIT_SPACING * 2, screenY,
                        ones, color);
    g_RenderState.packetCursor =
        QueueDrawModePrim(GamePrimaryOrderingTable(0), prim,
                          SPEED_DIGIT_TEXTURE_PAGE);
}
