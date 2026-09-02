#include "game/prim.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render_internal.h"


void DrawSpeedDigits(s32 x, s32 y, s32 value) {
    u8 *prim;
    s32 screenX;
    s32 screenY;
    s32 rawX;
    s32 rawY;
    s32 hundreds;
    s32 tens;
    s32 ones;
    u16 color;

    hundreds = value / 100;
    rawX = g_CarSpec->tachometer.digitsX + x;
    rawY = g_CarSpec->tachometer.digitsY + y;
    color = g_HudGlyphClut;
    prim = RENDER_PRIM_CURSOR_AS(u8);

    screenX = (s16)rawX;
    screenY = (s16)rawY;
    tens = (value / 10) % 10;
    ones = value % 10;

    prim = DrawHudDigit(prim, screenX, screenY, hundreds, color);
    prim = DrawHudDigit(prim, screenX + 8, screenY, tens, color);
    prim = DrawHudDigit(prim, screenX + 0x10, screenY, ones, color);
    g_RenderState.packetCursor =
        QueueDrawModePrim(GamePrimaryOrderingTable(0), prim, 9);
}
