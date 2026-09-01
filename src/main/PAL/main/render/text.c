#include "game/render_types.h"
#include "game/render_internal.h"

void DrawText8x8(s32 x, s32 y, const char *str, s32 clutIndex) {
    u8 **scratch = &RENDER_PRIM_CURSOR_AS(u8);
    RenderBufferAddress packet;

    packet.bytes = *scratch;
    if (*str != 0) {
        volatile SPRT_8 *sprt;
        u8 *font = g_Font8x8Cells;
        RenderBufferAddress spriteAddress;
        spriteAddress.bytes = packet.bytes;
        sprt = spriteAddress.volatileSprite8;
        do {
            s32 cell = *str - 0x20;

            str++;
            if (cell != 0) {
                s32 index;
                u8 *fontUCell;
                u8 *fontV;
                s32 u;
                s32 v;

                index = cell * 2;
                fontUCell = &font[index];
                fontV = &g_Font8x8Cells[1];
                u = *fontUCell * 8;
                v = *(index + fontV) * 8;

                SetSprt8(packet.bytes);
                SetShadeTex(packet.bytes, 1);
                sprt->x0 = x;
                sprt->y0 = y;
                sprt->u0 = u;
                sprt->v0 = v;
                sprt->clut = clutIndex;
                spriteAddress.volatileSprite8 = sprt;
                AddPrim(GamePrimaryOrderingTable(0), spriteAddress.pointer);
                sprt++;
                packet.bytes += sizeof(SPRT_8);
            }
            x += 8;
        } while (*str != 0);
    }
    SetDrawMode(packet.drawPacket, 0, 1, 9, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet.pointer);
    *scratch = packet.bytes + sizeof(DrawPacket);
}

void GameDrawText8x8Shaded(
    s32 x,
    s32 y,
    const char *str,
    s32 clutIndex,
    u8 intensity) {
    u8 **scratch = &RENDER_PRIM_CURSOR_AS(u8);
    RenderBufferAddress packet;
    u8 *prim;
    RenderBufferAddress primAddress;

    packet.bytes = *scratch;
    if (*str != 0) {
        volatile SPRT_8 *sprt;
        u8 *font = g_Font8x8Cells;

        primAddress.bytes = packet.bytes;
        sprt = primAddress.volatileSprite8;
        do {
            s32 cell = *str - 0x20;

            str++;
            if (cell != 0) {
                s32 u;
                s32 v;

                {
                    s32 index;
                    u8 *fontUCell;
                    u8 *fontV;

                    index = cell * 2;
                    fontUCell = &font[index];
                    fontV = &g_Font8x8Cells[1];
                    u = *fontUCell * 8;
                    v = *(index + fontV) * 8;
                }

                SetSprt8(packet.bytes);
                SetSemiTrans(packet.bytes, 1);
                sprt->x0 = x;
                sprt->y0 = y;
                sprt->u0 = u;
                sprt->v0 = v;
                sprt->r0 = intensity;
                sprt->g0 = intensity;
                sprt->b0 = intensity;
                primAddress.volatileSprite8 = sprt;
                prim = primAddress.bytes;
                sprt->clut = clutIndex;
                AddPrim(GamePrimaryOrderingTable(0), prim);
                sprt++;
                packet.bytes += sizeof(SPRT_8);
            }
            x += 8;
        } while (*str != 0);
    }
    SetDrawMode(packet.drawPacket, 0, 1, 0x29, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet.pointer);
    *scratch = packet.bytes + sizeof(DrawPacket);
}

/* Callers hold these strings as char *, so take them that way and read the
 * glyph codes unsigned. */
void DrawText8x8Trans(s32 x, s32 y, const char *text, s32 clutIndex) {
    const u8 *str = (const u8 *)text;
    u8 **scratch = &RENDER_PRIM_CURSOR_AS(u8);
    RenderBufferAddress packet;

    packet.bytes = *scratch;
    if (*str != 0) {
        volatile SPRT_8 *sprt;
        u8 *font = g_Font8x8Cells;
        RenderBufferAddress spriteAddress;
        spriteAddress.bytes = packet.bytes;
        sprt = spriteAddress.volatileSprite8;
        do {
            s32 cell = *str - 0x20;

            str++;
            if (cell != 0) {
                s32 index;
                u8 *fontUCell;
                u8 *fontV;
                s32 u;
                s32 v;

                index = cell * 2;
                fontUCell = &font[index];
                fontV = &g_Font8x8Cells[1];
                u = *fontUCell * 8;
                v = *(index + fontV) * 8;

                SetSprt8(packet.bytes);
                SetShadeTex(packet.bytes, 1);
                SetSemiTrans(packet.bytes, 1);
                sprt->x0 = x;
                sprt->y0 = y;
                sprt->u0 = u;
                sprt->v0 = v;
                sprt->clut = clutIndex;
                spriteAddress.volatileSprite8 = sprt;
                AddPrim(GamePrimaryOrderingTable(0), spriteAddress.pointer);
                sprt++;
                packet.bytes += sizeof(SPRT_8);
            }
            x += 8;
        } while (*str != 0);
    }
    SetDrawMode(packet.drawPacket, 0, 1, 0x49, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet.pointer);
    *scratch = packet.bytes + sizeof(DrawPacket);
}

#undef INIT_TEXT_FONT

/*
 * Local wide-parameter declarations. Retail passes x / y / clutIndex straight
 * through in full words; the s16 / u16 typing in game/render.h would make gcc
 * insert sign-extends and a truncation here, so this TU declares both the
 * callee and this function with s32 parameters instead of including the header.
 */
void GameDrawProportionalTextShaded(
    s32 x,
    s32 y,
    const char *str,
    s32 clutIndex,
    s32 intensity) {
typedef union TextRenderWork {
    s32 value;
    u8 *bytes;
} TextRenderWork;

#define OPAQUE_VALUE (t0.value = 0x100)
    s32 xPos = x;
    RenderBufferAddress packet;
    const u8 *text = (const u8 *)str;
    s32 shade;
    TextRenderWork t0;
    s32 s1;
    u32 first;
    s32 v;
    s32 u;
    struct {
        s32 y;
        s32 clut;
    } home;

    packet.bytes = RENDER_PRIM_CURSOR_AS(u8);
    home.y = y;
    home.clut = clutIndex;
    first = *text;
    shade = intensity;

    if (first != 0) {
        s32 height = 12;
        SPRT *sprt = packet.sprite;

        do {
            s32 advance;
            u32 ch = *text;

            if (ch >= 0x76) {
                s32 offset = ch - 0x76;
                s32 index = offset * 4;
                s32 width;
                SPRT *prim;
                s16 yOffset;

                
                text++;
                u = g_HighFontU[index];
                v = g_HighFontV[index];
                SetSprt(packet.bytes);
                if (shade == OPAQUE_VALUE) {
                    SetShadeTex(packet.bytes, 1);
                    sprt->x0 = xPos;
                } else {
                    SetSemiTrans(packet.bytes, 1);
                    sprt->r0 = shade;
                    sprt->g0 = shade;
                    sprt->b0 = shade;
                    sprt->x0 = xPos;
                }
                yOffset = g_HighFontYOffset[index];
                t0.value = home.y;
                
                packet.bytes += sizeof(SPRT);
                sprt->y0 = yOffset + t0.value;
                width = g_HighFontWidth[index];
                prim = sprt;
                sprt->u0 = u;
                sprt->v0 = v;
                /* RAW() keeps this store ahead of the g_DrawBuffer load --
                 * see common.h. */
                RAW(sprt->h) = height;
                t0.value = (u16)home.clut;
                sprt->clut = t0.value;
                sprt->w = width;
                AddPrim(GamePrimaryOrderingTable(0), prim);
                advance = g_WordFontWidth[index];
                sprt++;
                xPos += advance;
                continue;
            }
            if (ch >= 0x61) {
                s32 offset = ch - 0x61;
                s32 width;
                SPRT *prim;

                s1 = offset * 4;
                text++;
                u = g_WordFontU[s1];
                v = g_WordFontV[s1];
                SetSprt(packet.bytes);
                if (shade == OPAQUE_VALUE) {
                    SetShadeTex(packet.bytes, 1);
                    sprt->x0 = xPos;
                } else {
                    SetSemiTrans(packet.bytes, 1);
                    sprt->r0 = shade;
                    sprt->g0 = shade;
                    sprt->b0 = shade;
                    sprt->x0 = xPos;
                }
                t0.value = (u16)home.y;
                packet.bytes += sizeof(SPRT);
                sprt->y0 = t0.value;
                width = g_WordFontWidth[s1];
                prim = sprt;
                sprt->u0 = u;
                sprt->v0 = v;
                /* RAW() keeps this store ahead of the g_DrawBuffer load --
                 * see common.h. */
                RAW(sprt->h) = height;
                t0.value = (u16)home.clut;
                sprt->clut = t0.value;
                sprt->w = width;
                AddPrim(GamePrimaryOrderingTable(0), prim);
                advance = g_WordFontAdvance[s1];
                sprt++;
                xPos += advance;
                continue;
            }
            {
                s1 = ch - 0x20;

                
                text++;
                if (s1 != 0) {
                    s32 index = s1 * 2;
                    u8 *uCell;
                    u8 *vCell;
                    SPRT *prim;

                    t0.bytes = g_PropFontU;
                    uCell = index + t0.bytes;
                    t0.bytes = g_PropFontV;
                    vCell = index + t0.bytes;
                    
                    u = *uCell;
                    v = *vCell;
                    SetSprt(packet.bytes);
                    if (shade == OPAQUE_VALUE) {
                        SetShadeTex(packet.bytes, 1);
                        sprt->x0 = xPos;
                    } else {
                        SetSemiTrans(packet.bytes, 1);
                        sprt->r0 = shade;
                        sprt->g0 = shade;
                        sprt->b0 = shade;
                        sprt->x0 = xPos;
                    }
                    t0.value = (u16)home.y;
                    
                    prim = sprt;
                    
                    sprt->u0 = u;
                    sprt->v0 = v;
                    packet.bytes += sizeof(SPRT);
                    sprt->w = height;
                    sprt->h = height;
                    sprt->y0 = t0.value;
                    t0.value = (u16)home.clut;
                    sprt->clut = t0.value;
                    
                    sprt++;
                    AddPrim(GamePrimaryOrderingTable(0), prim);
                }
                xPos += 12;
            }
        } while (*text != 0);
    }
    SetDrawMode(packet.drawPacket, 0, 1, 0x29, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), packet.pointer);
    RENDER_PRIM_CURSOR_AS(u8) = packet.bytes + sizeof(DrawPacket);
#undef OPAQUE_VALUE
}


/* Opaque wrapper over GameDrawProportionalTextShaded: intensity 0x100 selects
 * the raw-texture (SetShadeTex) path instead of a modulated, semi-transparent
 * one. */
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex) {
    GameDrawProportionalTextShaded(x, y, str, clutIndex, 0x100);
}


/*
 * Local wide-parameter declaration: retail passes every coordinate, texel and
 * CLUT index as a full word (the stack arguments are read with `lw`), so the
 * narrow documentation types in game/render.h would make gcc shrink the loads.
 */
/* SPRT, 20 bytes: a raw (SetShadeTex) textured sprite linked into `ot`.
 * Returns the advanced packet cursor. */
u8 *GameQueueSprite(
    void *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex) {
    RenderBufferAddress address;
    SPRT *sprt;

    address.bytes = prim;
    sprt = address.sprite;

    SetSprt(sprt);
    SetShadeTex(sprt, 1);
    sprt->x0 = x;
    sprt->y0 = y;
    sprt->w = w;
    sprt->h = h;
    sprt->u0 = u;
    sprt->v0 = v;
    sprt->clut = clutIndex;
    prim += sizeof(*sprt);
    AddPrim(ot, sprt);
    return prim;
}

/* Local wide-parameter declaration; see GameQueueSprite.c. `clutIndex` really
 * is the narrow one here - retail reads it back out of its argument slot with
 * `lhu` at the point of use. */
/* SPRT, 20 bytes: a textured sprite modulated by `intensity` on all three
 * channels (no SetShadeTex, so the texel is shaded). */
u8 *GameQueueShadedSprite(
    void *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity) {
    RenderBufferAddress address;
    SPRT *sprt;

    address.bytes = prim;
    sprt = address.sprite;

    SetSprt(sprt);
    sprt->x0 = x;
    sprt->y0 = y;
    sprt->w = w;
    sprt->h = h;
    sprt->u0 = u;
    sprt->v0 = v;
    sprt->r0 = intensity;
    sprt->g0 = intensity;
    sprt->b0 = intensity;
    sprt->clut = clutIndex;
    prim += sizeof(*sprt);
    AddPrim(ot, sprt);
    return prim;
}

/* SPRT, 20 bytes: GameQueueShadedSprite plus SetSemiTrans. */
u8 *GameQueueShadedSpriteTrans(
    void *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity) {
    RenderBufferAddress address;
    SPRT *sprt;

    address.bytes = prim;
    sprt = address.sprite;

    SetSprt(sprt);
    SetSemiTrans(sprt, 1);
    sprt->x0 = x;
    sprt->y0 = y;
    sprt->w = w;
    sprt->h = h;
    sprt->u0 = u;
    sprt->v0 = v;
    sprt->r0 = intensity;
    sprt->g0 = intensity;
    sprt->b0 = intensity;
    sprt->clut = clutIndex;
    prim += sizeof(*sprt);
    AddPrim(ot, sprt);
    return prim;
}

/* SPRT, 20 bytes: GameQueueSprite plus SetSemiTrans. */
u8 *GameQueueSpriteTrans(
    void *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex) {
    RenderBufferAddress address;
    SPRT *sprt;

    address.bytes = prim;
    sprt = address.sprite;

    SetSprt(sprt);
    SetSemiTrans(sprt, 1);
    SetShadeTex(sprt, 1);
    sprt->x0 = x;
    sprt->y0 = y;
    sprt->w = w;
    sprt->h = h;
    sprt->u0 = u;
    sprt->v0 = v;
    sprt->clut = clutIndex;
    prim += sizeof(*sprt);
    AddPrim(ot, sprt);
    return prim;
}

/* TILE, 16 bytes: a semi-transparent solid rectangle linked into `ot`.
 * Returns the advanced packet cursor. */
u8 *GameQueueTileTrans(
    void *ot,
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
    prim += sizeof(*tile);
    AddPrim(ot, tile);
    return prim;
}

/*
 * Local wide-parameter declaration. Retail passes every coordinate and colour
 * component as a full word - the stack arguments are read with `lw` - so the
 * s16 / u8 typing that game/render.h uses for documentation would make gcc
 * narrow the loads here. Only this TU needs the wide view, so it declares the
 * symbol itself rather than changing the shared header.
 */
/* LINE_F2, 16 bytes: one flat-shaded line, linked into `ot`. Returns the
 * advanced packet cursor. */
u8 *GameQueueLine(
    void *ot,
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
    prim += sizeof(*line);
    AddPrim(ot, line);
    return prim;
}
