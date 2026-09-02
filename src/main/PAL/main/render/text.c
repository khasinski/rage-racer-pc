#include "game/render_types.h"
#include "game/render_internal.h"

enum {
    PROP_FONT_FIRST_CHARACTER = 0x20,
    PROP_FONT_GLYPH_COUNT = sizeof(g_PropFontCells) / 2,
    WORD_FONT_FIRST_CHARACTER = 'a',
    WORD_FONT_GLYPH_COUNT = sizeof(g_WordFontCells) / 4,
    HIGH_FONT_FIRST_CHARACTER = 'v',
    HIGH_FONT_GLYPH_COUNT = sizeof(g_HighFontCell) / 4,
};

static void QueueProportionalGlyph(
    RenderBufferAddress *packet,
    s32 x,
    s32 y,
    s32 u,
    s32 v,
    s32 width,
    s32 clut,
    s32 shade) {
    SPRT *sprite = packet->sprite;

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
    packet->sprite++;
}

void GameDrawProportionalTextShaded(
    s32 x,
    s32 y,
    const char *str,
    s32 clutIndex,
    s32 intensity) {
    s32 xPos = x;
    RenderBufferAddress packet;

    packet.bytes = RENDER_PRIM_CURSOR_AS(u8);

    while (*str != '\0') {
        u32 ch = (u8)*str++;
        s32 index;

        if (ch >= HIGH_FONT_FIRST_CHARACTER &&
            ch < HIGH_FONT_FIRST_CHARACTER + HIGH_FONT_GLYPH_COUNT) {
            index = (ch - HIGH_FONT_FIRST_CHARACTER) * 4;
            QueueProportionalGlyph(
                &packet, xPos, y + g_HighFontYOffset[index],
                g_HighFontU[index], g_HighFontV[index],
                g_HighFontWidth[index], clutIndex, intensity);
            xPos += g_WordFontWidth[index];
        } else if (ch >= WORD_FONT_FIRST_CHARACTER &&
                   ch < WORD_FONT_FIRST_CHARACTER + WORD_FONT_GLYPH_COUNT) {
            index = (ch - WORD_FONT_FIRST_CHARACTER) * 4;
            QueueProportionalGlyph(
                &packet, xPos, y, g_WordFontU[index], g_WordFontV[index],
                g_WordFontWidth[index], clutIndex, intensity);
            xPos += g_WordFontAdvance[index];
        } else if (ch >= PROP_FONT_FIRST_CHARACTER &&
                   ch < PROP_FONT_FIRST_CHARACTER + PROP_FONT_GLYPH_COUNT) {
            if (ch != ' ') {
                index = (ch - PROP_FONT_FIRST_CHARACTER) * 2;
                QueueProportionalGlyph(
                    &packet, xPos, y, g_PropFontU[index], g_PropFontV[index],
                    12, clutIndex, intensity);
            }
            xPos += 12;
        } else {
            continue;
        }
    }
    SetDrawMode(packet.drawPacket, 0, 1, 0x29, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet.drawPacket);
    packet.drawPacket++;
    g_RenderState.packetCursor = packet.bytes;
}


/* Opaque wrapper over GameDrawProportionalTextShaded: intensity 0x100 selects
 * the raw-texture (SetShadeTex) path instead of a modulated, semi-transparent
 * one. */
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex) {
    GameDrawProportionalTextShaded(x, y, str, clutIndex, 0x100);
}


static u8 *QueueSpritePacket(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity,
    s32 shadeTexture,
    s32 semiTransparent) {
    RenderBufferAddress address;
    SPRT *sprt;

    address.bytes = prim;
    sprt = address.sprite;

    SetSprt(sprt);
    SetShadeTex(sprt, shadeTexture);
    SetSemiTrans(sprt, semiTransparent);
    sprt->x0 = x;
    sprt->y0 = y;
    sprt->w = w;
    sprt->h = h;
    sprt->u0 = u;
    sprt->v0 = v;
    sprt->clut = clutIndex;
    if (!shadeTexture) {
        sprt->r0 = intensity;
        sprt->g0 = intensity;
        sprt->b0 = intensity;
    }
    AddPrim(ot, sprt);
    address.sprite++;
    return address.bytes;
}

/* SPRT, 20 bytes: a raw textured sprite linked into `ot`. */
u8 *GameQueueSprite(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex) {
    return QueueSpritePacket(
        ot, prim, x, y, w, h, u, v, clutIndex, 0, 1, 0);
}

/* SPRT, 20 bytes: a textured sprite modulated by `intensity` on all three
 * channels (no SetShadeTex, so the texel is shaded). */
u8 *GameQueueShadedSprite(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity) {
    return QueueSpritePacket(
        ot, prim, x, y, w, h, u, v, clutIndex, intensity, 0, 0);
}

/* SPRT, 20 bytes: GameQueueShadedSprite plus SetSemiTrans. */
u8 *GameQueueShadedSpriteTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity) {
    return QueueSpritePacket(
        ot, prim, x, y, w, h, u, v, clutIndex, intensity, 0, 1);
}

/* SPRT, 20 bytes: GameQueueSprite plus SetSemiTrans. */
u8 *GameQueueSpriteTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex) {
    return QueueSpritePacket(
        ot, prim, x, y, w, h, u, v, clutIndex, 0, 1, 1);
}

/* TILE, 16 bytes: a semi-transparent solid rectangle linked into `ot`.
 * Returns the advanced packet cursor. */
u8 *GameQueueTileTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 r,
    s32 g,
    s32 b) {
    RenderBufferAddress address;
    TILE *tile;

    address.bytes = prim;
    tile = address.tile;

    SetTile(tile);
    SetSemiTrans(tile, 1);
    tile->x0 = x;
    tile->y0 = y;
    tile->w = w;
    tile->h = h;
    tile->r0 = r;
    tile->g0 = g;
    tile->b0 = b;
    AddPrim(ot, tile);
    address.tile++;
    return address.bytes;
}

/* LINE_F2, 16 bytes: one flat-shaded line, linked into `ot`. Returns the
 * advanced packet cursor. */
u8 *GameQueueLine(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x0,
    s32 y0,
    s32 x1,
    s32 y1,
    s32 r,
    s32 g,
    s32 b) {
    RenderBufferAddress address;
    LINE_F2 *line;

    address.bytes = prim;
    line = address.lineF2;

    SetLineF2(line);
    line->x0 = x0;
    line->y0 = y0;
    line->x1 = x1;
    line->y1 = y1;
    line->r0 = r;
    line->g0 = g;
    line->b0 = b;
    AddPrim(ot, line);
    address.lineF2++;
    return address.bytes;
}
