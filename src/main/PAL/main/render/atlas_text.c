#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

enum {
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
    const s32 fixedWidth = (flags & DRAW_ATLAS_TEXT_FIXED_WIDTH) != 0;

    while (*cursor != '\0') {
        s32 glyphIndex;
        s32 glyphWidth;
        s32 textureU;
        s32 textureV;
        FontGlyph glyph;

        if (*cursor == ' ') {
            cursor++;
            x = WrapSigned32((int64_t)x + 6);
            continue;
        }

        glyphIndex = SmallGlyph(&cursor);
        if (glyphIndex < 0) {
            continue;
        }

        glyph = g_SmallFontGlyphs[glyphIndex];
        glyphWidth = fixedWidth ? 6 : glyph.width;
        textureU = fixedWidth ? (glyphIndex % 42) * 6
                              : glyph.u;
        textureV = fixedWidth ? (glyphIndex / 42) * 12
                              : glyph.v;
        DrawSprite(ot + 1, WrapSigned16(x), y,
                   WrapSigned16(glyphWidth), 12, (u16)textureU,
                   (u16)textureV, red, green, blue, clut, 0, 1, 0x80);
        x = WrapSigned32((int64_t)x + glyphWidth);
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot + 1, RENDER_PRIM_CURSOR_AS(void),
        (flags & TEXT_TEXTURE_PAGE_MASK) + TEXT_TEXTURE_PAGE_BASE);
}

void DrawLargeText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags) {
    const u8 *cursor = (const u8 *)text;
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    const s32 fixedWidth = (flags & DRAW_ATLAS_TEXT_FIXED_WIDTH) != 0;

    while (*cursor != '\0') {
        s32 glyphIndex;
        s32 glyphWidth;
        s32 textureU;
        s32 textureV;
        const u8 character = *cursor++;
        FontGlyph glyph;

        if (character == ' ') {
            x = WrapSigned32((int64_t)x + 8);
            continue;
        }

        glyphIndex = LargeGlyph(character);
        if (glyphIndex < 0) {
            continue;
        }

        glyph = g_LargeFontGlyphs[glyphIndex];
        glyphWidth = fixedWidth ? 8 : glyph.width;
        textureU = fixedWidth ? (glyphIndex % 32) * 8
                              : glyph.u;
        textureV = fixedWidth ? (glyphIndex / 32) * 16 + 24
                              : glyph.v;
        DrawSprite(ot + 1, WrapSigned16(x), y,
                   WrapSigned16(glyphWidth), 16, (u16)textureU,
                   (u16)textureV, red, green, blue, clut, 0, 1, 0x80);
        x = WrapSigned32((int64_t)x + glyphWidth);
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot + 1, RENDER_PRIM_CURSOR_AS(void),
        (flags & TEXT_TEXTURE_PAGE_MASK) + TEXT_TEXTURE_PAGE_BASE);
}
