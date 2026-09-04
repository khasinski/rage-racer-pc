#include <sys/types.h>

#include "game/prim.h"
#include "game/render_internal.h"
#include "game/render_types.h"


void DrawSpriteString(s32 x, s32 y, const char *str, s32 clutIndex) {
    u8 *packet = RENDER_PRIM_CURSOR_AS(u8);
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);

    while (*str != '\0') {
        s32 glyph = PrintableAsciiGlyph((u8)*str++);
        s32 width = g_SpriteFontWidth[glyph];

        if (glyph != 0) {
            SPRT *sprite = (SPRT *)packet;
            SpriteFontCell cell = g_SpriteFontCells[glyph];

            SetSprt(sprite);
            SetShadeTex(sprite, 1);
            sprite->x0 = WrapSigned16(x);
            sprite->y0 = WrapSigned16(y);
            sprite->u0 = cell.textureU;
            sprite->v0 = cell.textureV;
            sprite->w = (u16)width;
            sprite->h = 0x18;
            sprite->clut = (u16)clutIndex;
            AddPrim(ot, sprite);
            packet = (u8 *)(sprite + 1);
        }
        x = WrapSigned32((int64_t)x + width);
    }

    SetDrawMode((DrawPacket *)packet, 0, 1, 0x1D, g_DrawModeEnv);
    AddPrim(ot, packet);
    g_RenderState.packetCursor = (DrawPacket *)packet + 1;
}

u8 *DrawShadowedTile(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y) {
    u8 *next;

    next = AddTilePrim(
        ot, prim,
        WrapSigned32((int64_t)x + 1),
        WrapSigned32((int64_t)y + 2),
        0xC2, 0x1C, 0, 0, 0);
    return AddTilePrim(ot, next, x, y, 0xC4, 0x20, 0xFF, 0xFF, 0xFF);
}
