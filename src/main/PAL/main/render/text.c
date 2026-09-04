#include "game/render_types.h"
#include "game/render_internal.h"

#include <string.h>

typedef struct WordFontCell {
    u8 textureU;
    u8 textureV;
    u8 width;
    u8 advance;
} WordFontCell;

typedef struct CurrencyFontCell {
    u8 textureU;
    u8 textureV;
    u8 width;
    u8 yOffset;
} CurrencyFontCell;

enum {
    PROP_FONT_FIRST_CHARACTER = 0x20,
    PROP_FONT_GLYPH_COUNT = PROPORTIONAL_FONT_CELL_COUNT,
    WORD_FONT_FIRST_CHARACTER = 'a',
    WORD_FONT_GLYPH_COUNT =
        sizeof(g_WordFontCells) / sizeof(WordFontCell),
    CURRENCY_FONT_CHARACTER = 'v',
};

_Static_assert(sizeof(g_HighFontCell) == sizeof(CurrencyFontCell),
               "currency font data must contain exactly one cell");

static WordFontCell WordFontCellAt(u32 index) {
    WordFontCell cell;

    memcpy(&cell, g_WordFontCells + index * sizeof(cell), sizeof(cell));
    return cell;
}

static CurrencyFontCell CurrencyFontCellValue(void) {
    CurrencyFontCell cell;

    memcpy(&cell, g_HighFontCell, sizeof(cell));
    return cell;
}

static u8 *QueueProportionalGlyph(
    u8 *packet,
    s32 x,
    s32 y,
    s32 u,
    s32 v,
    s32 width,
    s32 clut,
    s32 shade) {
    SPRT *sprite = (SPRT *)packet;

    SetSprt(sprite);
    if (shade == 0x100) {
        SetShadeTex(sprite, 1);
    } else {
        SetSemiTrans(sprite, 1);
        sprite->r0 = shade;
        sprite->g0 = shade;
        sprite->b0 = shade;
    }
    sprite->x0 = x;
    sprite->y0 = y;
    sprite->u0 = u;
    sprite->v0 = v;
    sprite->w = width;
    sprite->h = 12;
    sprite->clut = clut;
    AddPrim(GamePrimaryOrderingTable(0), sprite);
    return (u8 *)(sprite + 1);
}

void GameDrawProportionalTextShaded(
    s32 x,
    s32 y,
    const char *str,
    s32 clutIndex,
    s32 intensity) {
    CurrencyFontCell currencyCell = CurrencyFontCellValue();
    WordFontCell firstWordCell = WordFontCellAt(0);
    s32 xPos = x;
    u8 *packet = RENDER_PRIM_CURSOR_AS(u8);

    while (*str != '\0') {
        u32 ch = (u8)*str++;

        if (ch == CURRENCY_FONT_CHARACTER) {
            packet = QueueProportionalGlyph(
                packet, xPos, y + currencyCell.yOffset,
                currencyCell.textureU, currencyCell.textureV,
                currencyCell.width, clutIndex, intensity);
            /* Retail uses the first word cell's width after the terminal
             * currency marker. The shipped format strings place it last. */
            xPos += firstWordCell.width;
        } else if (ch >= WORD_FONT_FIRST_CHARACTER &&
                   ch < WORD_FONT_FIRST_CHARACTER + WORD_FONT_GLYPH_COUNT) {
            WordFontCell cell =
                WordFontCellAt(ch - WORD_FONT_FIRST_CHARACTER);

            packet = QueueProportionalGlyph(
                packet, xPos, y, cell.textureU, cell.textureV,
                cell.width, clutIndex, intensity);
            xPos += cell.advance;
        } else if (ch >= PROP_FONT_FIRST_CHARACTER &&
                   ch < PROP_FONT_FIRST_CHARACTER + PROP_FONT_GLYPH_COUNT) {
            if (ch != ' ') {
                ProportionalFontCell cell =
                    g_PropFontCells[ch - PROP_FONT_FIRST_CHARACTER];

                packet = QueueProportionalGlyph(
                    packet, xPos, y, cell.textureU, cell.textureV, 12,
                    clutIndex, intensity);
            }
            xPos += 12;
        } else {
            continue;
        }
    }
    SetDrawMode((DrawPacket *)packet, 0, 1, 0x29, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet);
    g_RenderState.packetCursor = (DrawPacket *)packet + 1;
}


/* Opaque wrapper over GameDrawProportionalTextShaded: intensity 0x100 selects
 * the raw-texture (SetShadeTex) path instead of a modulated, semi-transparent
 * one. */
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex) {
    GameDrawProportionalTextShaded(x, y, str, clutIndex, 0x100);
}
