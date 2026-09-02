#include "game/render.h"
#include "game/render_internal.h"

/* DR_MODE, 12 bytes: sets the texture page (and the blend mode packed into it)
 * for the primitives that follow, links it into the ordering table and returns
 * the advanced packet cursor. */
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packetCursor,
                      s32 tpage) {
    RenderBufferAddress cursor;
    u8 *packetStart = packetCursor;

    cursor.bytes = packetCursor;
    SetDrawMode(cursor.drawPacket, 0, 1, (u16)tpage, g_DrawModeEnv);
    packetCursor += sizeof(DrawPacket);
    AddPrim(ot, packetStart);
    return packetCursor;
}

u8 *GameQueueShadedTexturedRect(GameOrderingTableEntry *ot, u8 *packetCursor,
                                s32 x, s32 y, s32 w, s32 h, s32 u, s32 v,
                                s32 clutIndex, s32 tpage, s32 intensity) {
    POLY_FT4 *packet;
    RenderBufferAddress packetAddress;
    s16 width = w;
    s16 height = h;
    u8 textureU = u;
    u8 textureV = v;

    packetAddress.bytes = packetCursor;
    SetPolyFT4(packetAddress.polyFT4);
    if (width < 0) {
        width += 1;
        textureU -= width;
    }
    if (height < 0) {
        height += 1;
        textureV -= height;
    }

    packet = packetAddress.polyFT4;
    packet->x0 = x;
    packet->y0 = y;
    packet->x1 = x + (width < 0 ? -width : width);
    packet->y1 = y;
    packet->x2 = x;
    packet->y2 = y + (height < 0 ? -height : height);
    packet->x3 = x + (width < 0 ? -width : width);
    packet->y3 = y + (height < 0 ? -height : height);
    packet->u0 = textureU;
    packet->v0 = textureV;
    packet->u1 = textureU + width;
    packet->v1 = textureV;
    packet->u2 = textureU;
    packet->v2 = textureV + height;
    packet->u3 = textureU + width;
    packet->v3 = textureV + height;
    packet->r0 = intensity;
    packet->g0 = intensity;
    packet->b0 = intensity;
    packet->clut = clutIndex;
    packet->tpage = tpage;
    packetCursor += sizeof(POLY_FT4);
    AddPrim(ot, packet);
    return packetCursor;
}

u8 *GameQueueTexturedRect(GameOrderingTableEntry *ot, u8 *packetCursor, s32 x,
                          s32 y, s32 w, s32 h, s32 u, s32 v, s32 uSpan,
                          s32 vSpan, s32 clutIndex, s32 tpage) {
    POLY_FT4 *packet;
    RenderBufferAddress packetAddress;
    s16 width = w;
    s16 height = h;
    u8 textureU = u;
    u8 textureV = v;

    packetAddress.bytes = packetCursor;
    SetPolyFT4(packetAddress.polyFT4);
    SetShadeTex(packetAddress.polyFT4, 1);

    if (width < 0) {
        textureU -= width + 1;
    }
    if (height < 0) {
        textureV -= height + 1;
    }

    packet = packetAddress.polyFT4;
    packet->x0 = x;
    packet->y0 = y;
    packet->x1 = x + (width < 0 ? -width : width);
    packet->y1 = y;
    packet->x2 = x;
    packet->y2 = y + (height < 0 ? -height : height);
    packet->x3 = x + (width < 0 ? -width : width);
    packet->y3 = y + (height < 0 ? -height : height);
    packet->u0 = textureU;
    packet->v0 = textureV;
    packet->u1 = textureU + uSpan;
    packet->v1 = textureV;
    packet->u2 = textureU;
    packet->v2 = textureV + vSpan;
    packet->u3 = textureU + uSpan;
    packet->v3 = textureV + vSpan;
    packet->clut = clutIndex;
    packet->tpage = tpage;
    packetCursor += sizeof(POLY_FT4);
    AddPrim(ot, packet);
    return packetCursor;
}
