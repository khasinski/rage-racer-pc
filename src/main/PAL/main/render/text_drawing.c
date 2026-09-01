#include "game/prim.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/render.h"



void DrawSmallText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut, s32 flags) {
    const char *str;
    s32 x;
    OT_TYPE *ot;
    s32 fixed = flags & 0x80;
    s32 idx;
    s32 u0;
    s32 v0;
    s32 w;
    u8 c;
    s32 c2;

    str = str0;
    x = x0;
    ot = RENDER_OT_BASE_AS(OT_TYPE);

    while (*str) {
        c = *str;
        str++;
        switch (c) {
        case ' ':
            x += 6;
            continue;
        case '/':
            idx = 0x24;
            break;
        case '.':
            idx = 0x25;
            break;
        case ',':
            idx = 0x26;
            break;
        case '"':
            idx = 0x27;
            break;
        case '\'':
            idx = 0x28;
            break;
        case '-':
            idx = 0x29;
            break;
        case 0x81:
            c2 = *str;
            str++;
            switch (c2) {
            case 0x9b:
                idx = 0x2a;
                break;
            case 0xa0:
                idx = 0x2b;
                break;
            case 0x7e:
                idx = 0x2c;
                break;
            case 0xa2:
                idx = 0x2d;
                break;
            default:
                continue;
            }
            break;
        default:
            if (c < '0') {
                continue;
            }
            if (c < ':') {
                idx = c - '0';
            } else if (c < 'A') {
                continue;
            } else if (c < '[') {
                idx = c - '7';
            } else {
                continue;
            }
        }

        w = fixed ? 6 : g_SmallFontGlyphs[idx].width;
        u0 = fixed ? (idx % 42) * 6 : g_SmallFontGlyphs[idx].u;
        v0 = fixed ? (idx / 42) * 12 : g_SmallFontGlyphs[idx].v;

        DrawSprite(
            ot + 1,
            (s16)x,
            (s16)y,
            (s16)w,
            0xc,
            (s16)u0,
            (s16)v0,
            (u8)color,
            g,
            b,
            clut,
            0,
            1,
            0x80);
        x += w;
    }

    RENDER_PRIM_CURSOR_AS(void) = QueueDrawModePrim(
        ot + 1, RENDER_PRIM_CURSOR_AS(u8), (flags & 0x7f) + 27);
}


void DrawLargeText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut, s32 flags) {
    const char *str;
    s32 x;
    s32 fixed = flags & 0x80;
    OT_TYPE *ot;
    s32 idx;
    s32 u0;
    s32 v0;
    s32 w;
    u8 c;

    str = str0;
    x = x0;
    ot = RENDER_OT_BASE_AS(OT_TYPE);

    while (*str) {
        c = *str;
        str++;
        switch (c) {
        case ' ':
            x += 8;
            continue;
        case '.':
            idx = 0x24;
            break;
        case '-':
            idx = 0x25;
            break;
        case '!':
            idx = 0x26;
            break;
        case '?':
            idx = 0x27;
            break;
        case '@':
            idx = 0x28;
            break;
        case '/':
            idx = 0x2b;
            break;
        case ',':
            idx = 0x2c;
            break;
        case '"':
            idx = 0x2d;
            break;
        case '\'':
            idx = 0x2e;
            break;
        case ':':
            idx = 0x30;
            break;
        default:
            if (c < '0') {
                continue;
            }
            if (c < ':') {
                idx = c - '0';
            } else if (c < 'A') {
                continue;
            } else if (c < '[') {
                idx = c - '7';
            } else {
                continue;
            }
        }

        w = fixed ? 8 : g_LargeFontGlyphs[idx].width;
        u0 = fixed ? (idx % 32) * 8 : g_LargeFontGlyphs[idx].u;
        v0 = fixed ? (idx / 32) * 16 + 24 : g_LargeFontGlyphs[idx].v;

        DrawSprite(
            ot + 1,
            (s16)x,
            (s16)y,
            (s16)w,
            0x10,
            (s16)u0,
            (s16)v0,
            (u8)color,
            g,
            b,
            clut,
            0,
            1,
            0x80);
        x += w;
    }

    RENDER_PRIM_CURSOR_AS(void) =
        QueueDrawModePrim(ot + 1, RENDER_PRIM_CURSOR_AS(void),
                          (flags & 0x7f) + 27);
}


s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 r, u8 g, u8 b,
                   u16 clut, u8 primitiveCount) {
    u8 digits[10];
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE) +
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

    RENDER_PRIM_CURSOR_AS(void) =
        QueueDrawModePrim(ot, RENDER_PRIM_CURSOR_AS(void), primitiveCount + 27);
    return 10 - firstDigit;
}


void DrawBitPatternOverlay(s32 pattern) {
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE);
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

    RENDER_PRIM_CURSOR_AS(void) = QueueDrawModePrim(ot + 1, RENDER_PRIM_CURSOR_AS(void), 0x39);
}
