#include "common.h"
#include <stdio.h>
#include "game/asset.h"
#include "game/menu.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/scratchpad.h"

void DrawEngineSpecLabel(s32 x, s32 y, s32 label) {
    DrawText8x8(x, y, g_EngineSpecLabels[label], 0x78CC);
}


void DrawCarEngineSpec(s32 slideRaw, s32 brightness) {
    OT_TYPE *ot;
    u8 buf[0x40];
    s32 slide;
    s32 n;
    s32 p;
    s32 q;
    s32 r;
    s32 torqueEnd;

    ot = SCRATCH_OT_BASE_AS(OT_TYPE) + 1;
    if (g_MenuAltLayout != 0) {
        return;
    }
    slide = (s16)slideRaw;
    DrawSprite(ot, 0xA1, 0xCC - slide, 0x10, 0xC, 0, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, 0xB2, 0xCC - slide, 0x1C, 0xC, 0x10, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    n = sprintf(buf, g_FormatDecimal, g_CarModelAsset->maxPower);
    DrawSmallText(0xD2, 0xCB - slide, buf, (u8)brightness, (u8)brightness, (u8)brightness, 0x244,
                  0x20);
    p = n * 6;
    DrawSprite(ot, p + 0xD4, 0xCC - slide, 8, 0xC, 0x70, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, p + 0xDF, 0xCC - slide, 6, 0xC, 0xD8, 0, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3B);
    q = p + 0xE6;
    n = sprintf(buf, g_FormatDecimal, g_CarModelAsset->maxPowerRpm);
    DrawSmallText(q, 0xCB - slide, buf, (u8)brightness, (u8)brightness, (u8)brightness, 0x244,
                  0x20);
    torqueEnd = p + 0xE8;
    DrawSprite(ot, torqueEnd + (n * 6), 0xCC - slide, 0x10, 0xC, 0x78, 0xF4, (u8)brightness,
               (u8)brightness, (u8)brightness, 0x244, 0, 1, 0x3A);

    DrawSprite(ot, 0xA1, 0xDA - slide, 0x10, 0xC, 0, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, 0xB2, 0xDA - slide, 0x20, 0xC, 0x2C, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    n = sprintf(buf, g_FormatDecimal, g_CarModelAsset->maxTorqueWhole);
    DrawSmallText(0xD2, 0xD9 - slide, buf, (u8)brightness, (u8)brightness, (u8)brightness, 0x244,
                  0x20);
    p = n * 6;
    DrawSprite(ot, p + 0xD3, 0xDA - slide, 3, 0xC, 0xE0, 0, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3B);
    q = p + 0xD5;
    n = sprintf(buf, g_FormatDecimal, g_CarModelAsset->maxTorqueFraction);
    DrawSmallText(q, 0xD9 - slide, buf, (u8)brightness, (u8)brightness, (u8)brightness, 0x244,
                  0x20);
    p += 0xD7;
    /* Written as an operand swap: the target keeps the string length in
       the higher callee-saved than the running cursor. */
    q = n;
    n = p;
    q = n + (q * 6);
    DrawSprite(ot, q, 0xDA - slide, 0x10, 0xC, 0x88, 0xF4, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, q + 0x11, 0xDA - slide, 6, 0xC, 0xD8, 0, (u8)brightness, (u8)brightness,
               (u8)brightness, 0x244, 0, 1, 0x3B);
    q += 0x18;
    n = sprintf(buf, g_FormatDecimal, g_CarModelAsset->maxTorqueRpm);
    DrawSmallText(q, 0xD9 - slide, buf, (u8)brightness, (u8)brightness, (u8)brightness, 0x244,
                  0x20);
    r = q + 2;
    DrawSprite(ot, r + (n * 6), 0xDA - slide, 0x10, 0xC, 0x78, 0xF4, (u8)brightness,
               (u8)brightness, (u8)brightness, 0x244, 0, 1, 0x3A);
}
