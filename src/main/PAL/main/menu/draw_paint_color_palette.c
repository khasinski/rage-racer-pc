#include "common.h"
#include "game/menu_internal.h"
#include "game/menu_types.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/vector.h"


/* The 18-swatch PAINT COLOR strip with its selection frame and enlarged preview. */
s32 DrawPaintColorPalette(s32 *counter, s32 step, s32 index) {
    PaintColorTable localTable;
    void *ot;
    s32 progress;
    s32 wideOffset;
    s32 phase;
    PaintColorTable *tableSource;
    s32 yBase;
    s32 xBase;
    u8 sineByte;
    u16 xBaseHalf;
    s32 yBorder;
    s32 yTop;
    s32 xFocus;
    s32 xFocusOffset;
    s32 sineColor;
    s32 xOffset;
    s32 i;
    s32 colorIndex;
    s32 next;
    PaintColorTable *srcTable;
    Rgb *color;

    ot = SCRATCH_OT_BASE;
    srcTable = &g_PaintColorTable;
    tableSource = srcTable;
    localTable = *tableSource;

    if (step < 0) {
        *counter += step;
        if (*counter < 0) {
            *counter = 0;
        }
    }

    progress = *counter - 11;
    wideOffset = (g_MenuAltLayout != 0) ? 0x2C : 0;

    if (progress >= 0) {
        if (progress >= 11) {
            progress = 10;
        }

        phase = g_PaintPalettePulsePhase * 2;
        xBase = 0x9E - wideOffset;
        yBase = ((u32)-(progress * 480)) >> 5;
        xBaseHalf = xBase;
        yBorder = yBase + 0x20E;
        xFocusOffset = index * 8;
        xFocusOffset -= 2;
        xFocus = xBase + xFocusOffset;
        yTop = yBase + 0x20B;

        sineColor = rsin(phase % 0x1000);
        if (sineColor < 0) {
            sineColor += 0x3F;
        }
        sineColor = (sineColor >> 6) - 0x41;

        g_PaintPalettePulsePhase += 0x20;

        DrawRectOutline(ot, (s16)xFocus, (s16)yTop, 0xD, 0x1A, 0,
                      sineByte = (u8)sineColor, 0, (u8)0xFF);

        color = &localTable.colors[index];
        DrawSolidRect(ot, (s16)(xFocus + 1), (s16)(yBase + 0x20D), 0xB, 0x16, color->r,
                      color->g, color->b, 0xFF);

        DrawRectOutline(ot, xBase, (s16)yBorder, 0x92, 0x14,
                        (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);

        xOffset = 0;
        i = 0;
        colorIndex = 1;
        do {
            PaintColorAddress colorAddress;

            colorAddress.pointer = localTable.colors;
            colorAddress.bytes += i;
            DrawSolidRect(ot, (s16)(xBaseHalf + colorIndex), (s16)(yBase + 0x210), 8,
                          0x10, colorAddress.pointer->r, colorAddress.pointer->g,
                          colorAddress.pointer->b, 0xFF);
            i += 3;
            colorIndex += 8;
            xOffset++;
        } while (xOffset < MENU_PAINT_COLOR_COUNT);
    }

    if (step >= 0) {
        next = step + *counter;
        if (next >= 25) {
            *counter = 25;
            return 1;
        }
        *counter = next;
    }

    return 0;
}


void DrawOwnedCarCounter(s32 owned, s32 step) {
    s32 ownedCount;
    s32 a1v;
    void *ot;
    s32 v0, v1, t, t2;
    s16 y;

    ownedCount = owned;
    ot = SCRATCH_OT_BASE;
    a1v = step;

    if (ownedCount == 0) {
        g_OwnedCarCounterSlide = 0;
        return;
    }
    if (ownedCount < 0) {
        v0 = ownedCount + g_OwnedCarCounterSlide;
        g_OwnedCarCounterSlide = v0;
        if (v0 < 0) {
            g_OwnedCarCounterSlide = 0;
        }
    }
    v0 = g_OwnedCarCounterSlide;
    v1 = v0 - 11;
    if (v1 >= 0 && g_MenuAltLayout == 0) {
        u32 slidePhase;

        if (v1 >= 11) {
            v1 = 10;
        }
        slidePhase = v1;
        t = (slidePhase * -1120) >> 5;
        y = t + 0x21B;
        t2 = t + 0x211;
        GameDrawNumber(0x2C, y, 7, a1v, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
        GameDrawNumber(0x44, y, 7, 0xD, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
        DrawSprite(ot, 0x17, y, 0x34, 0x10, 0x8C, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0x7C, y, 0x8, 0x10, 0x8C, 0xDC, 0, 0, 0, 0x259, 1, 1, 0x3B);
        GameDrawMenuButton(0, (s16)t2, 0x99, 0x23, 0, 0, 0, 0, 0, 0, 0);
    }
    if (ownedCount > 0) {
        v0 = ownedCount + g_OwnedCarCounterSlide;
        g_OwnedCarCounterSlide = v0;
        if (v0 >= 0x1A) {
            g_OwnedCarCounterSlide = 0x19;
        }
    }
}
