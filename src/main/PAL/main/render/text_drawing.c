#include "game/prim.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/render.h"
s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 r, u8 g, u8 b,
                   u16 clut, u8 primitiveCount) {
    u8 digits[10];
    GameOrderingTableEntry *ot = RENDER_OT_BASE +
                  ((flags & DRAW_NUMBER_OVERLAY_LAYER) != 0);
    s32 width;
    s32 height;
    s32 textureV;
    s32 firstDigit;
    s32 fieldStart;
    s32 digitIndex;

    if (flags & DRAW_NUMBER_ALT_DIGIT_ATLAS) {
        width = 8;
        height = 16;
        textureV = 0xDC;
    } else {
        const s32 useLargeDigits = flags & DRAW_NUMBER_LARGE_DIGITS;
        width = useLargeDigits ? 8 : 6;
        height = useLargeDigits ? 16 : 12;
        textureV = useLargeDigits ? 0x18 : 0;
    }

    for (digitIndex = 9; digitIndex >= 0; digitIndex--) {
        digits[digitIndex] = value % 10;
        value /= 10;
    }

    firstDigit = 0;
    while (firstDigit < 9 && digits[firstDigit] == 0) {
        firstDigit++;
    }

    fieldStart = (flags & DRAW_NUMBER_TEN_DIGIT_FIELD) != 0 ? 0 : firstDigit;
    x += (firstDigit - fieldStart) * width;
    for (digitIndex = firstDigit; digitIndex < 10; digitIndex++) {
        DrawSprite(ot, (s16)x, y, width, height, digits[digitIndex] * width,
                   textureV, r, g, b, clut, 0, 1, 0x80);
        x += width;
    }

    g_RenderState.packetCursor =
        QueueDrawModePrim(ot, RENDER_PRIM_CURSOR_AS(void), primitiveCount + 27);
    return 10 - firstDigit;
}

void DrawBitPatternOverlay(s32 pattern) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    u8 *patternTable = g_MenuOverlayPatternTable;
    u8 *row = patternTable;
    u8 *candidate;
    s32 patternRow;
    s32 bit;

    if (pattern == 0) {
        return;
    }

    if (pattern < 0) {
        if ((g_AnimTimer % 6U) == 0) {
            g_MenuOverlayPatternAnimOffset += 8;
        }

        candidate = &patternTable[g_MenuOverlayPatternAnimOffset];
        if (candidate[7] != 0) {
            g_MenuOverlayPatternAnimOffset = 0x10;
        }
        row = &patternTable[g_MenuOverlayPatternAnimOffset];
    } else {
        row = &patternTable[(pattern - 1) * 8];
    }

    for (patternRow = 0; patternRow < 6; patternRow++, row++) {
        for (bit = 0; bit < 8; bit++) {
            if (((row[0] << bit) & 0x80) != 0) {
                DrawSprite(
                    ot + 1,
                    (s16)(0x22 + bit * 4),
                    (s16)(0x150 + patternRow * 8),
                    4,
                    8,
                    0xFC,
                    0,
                    0,
                    0,
                    0,
                    0x244,
                    1,
                    1,
                    0x80);
            }
        }
    }

    for (bit = 0; bit < 16; bit++) {
        DrawSprite(
            ot + 1,
            (s16)(0x4C + bit * 5),
            0x33,
            4,
            8,
            0xFC,
            0,
            0,
            0,
            0,
            0x244,
            1,
            1,
            0x80);
    }

    g_RenderState.packetCursor = QueueDrawModePrim(ot + 1, RENDER_PRIM_CURSOR_AS(void), 0x39);
}
