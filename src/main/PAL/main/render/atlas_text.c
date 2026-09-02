#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

enum {
    TEXT_FIXED_WIDTH = 0x80,
    TEXT_TEXTURE_PAGE_MASK = 0x7f,
    TEXT_TEXTURE_PAGE_BASE = 27,
};

static s32 LetterOrDigitGlyph(u8 character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'A' && character <= 'Z') {
        return character - '7';
    }
    return -1;
}

static s32 SmallGlyph(const u8 **text) {
    const u8 character = *(*text)++;

    switch (character) {
    case '/': return 0x24;
    case '.': return 0x25;
    case ',': return 0x26;
    case '"': return 0x27;
    case '\'': return 0x28;
    case '-': return 0x29;
    case 0x81:
        if (**text == '\0') {
            return -1;
        }
        switch (*(*text)++) {
        case 0x9b: return 0x2a;
        case 0xa0: return 0x2b;
        case 0x7e: return 0x2c;
        case 0xa2: return 0x2d;
        default: return -1;
        }
    default:
        return LetterOrDigitGlyph(character);
    }
}

static s32 LargeGlyph(u8 character) {
    switch (character) {
    case '.': return 0x24;
    case '-': return 0x25;
    case '!': return 0x26;
    case '?': return 0x27;
    case '@': return 0x28;
    case '/': return 0x2b;
    case ',': return 0x2c;
    case '"': return 0x2d;
    case '\'': return 0x2e;
    case ':': return 0x30;
    default:
        return LetterOrDigitGlyph(character);
    }
}

void DrawSmallText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags) {
    const u8 *cursor = (const u8 *)text;
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    const s32 fixedWidth = (flags & TEXT_FIXED_WIDTH) != 0;

    while (*cursor != '\0') {
        s32 glyphIndex;
        s32 glyphWidth;
        s32 textureU;
        s32 textureV;

        if (*cursor == ' ') {
            cursor++;
            x += 6;
            continue;
        }

        glyphIndex = SmallGlyph(&cursor);
        if (glyphIndex < 0) {
            continue;
        }

        glyphWidth = fixedWidth ? 6 : g_SmallFontGlyphs[glyphIndex].width;
        textureU = fixedWidth ? (glyphIndex % 42) * 6
                              : g_SmallFontGlyphs[glyphIndex].u;
        textureV = fixedWidth ? (glyphIndex / 42) * 12
                              : g_SmallFontGlyphs[glyphIndex].v;
        DrawSprite(ot + 1, (s16)x, y, (s16)glyphWidth, 12, (s16)textureU,
                   (s16)textureV, red, green, blue, clut, 0, 1, 0x80);
        x += glyphWidth;
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot + 1, RENDER_PRIM_CURSOR_AS(void),
        (flags & TEXT_TEXTURE_PAGE_MASK) + TEXT_TEXTURE_PAGE_BASE);
}

void DrawLargeText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags) {
    const u8 *cursor = (const u8 *)text;
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    const s32 fixedWidth = (flags & TEXT_FIXED_WIDTH) != 0;

    while (*cursor != '\0') {
        s32 glyphIndex;
        s32 glyphWidth;
        s32 textureU;
        s32 textureV;
        const u8 character = *cursor++;

        if (character == ' ') {
            x += 8;
            continue;
        }

        glyphIndex = LargeGlyph(character);
        if (glyphIndex < 0) {
            continue;
        }

        glyphWidth = fixedWidth ? 8 : g_LargeFontGlyphs[glyphIndex].width;
        textureU = fixedWidth ? (glyphIndex % 32) * 8
                              : g_LargeFontGlyphs[glyphIndex].u;
        textureV = fixedWidth ? (glyphIndex / 32) * 16 + 24
                              : g_LargeFontGlyphs[glyphIndex].v;
        DrawSprite(ot + 1, (s16)x, y, (s16)glyphWidth, 16, (s16)textureU,
                   (s16)textureV, red, green, blue, clut, 0, 1, 0x80);
        x += glyphWidth;
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot + 1, RENDER_PRIM_CURSOR_AS(void),
        (flags & TEXT_TEXTURE_PAGE_MASK) + TEXT_TEXTURE_PAGE_BASE);
}
