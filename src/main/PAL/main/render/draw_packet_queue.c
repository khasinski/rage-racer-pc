#include "game/render.h"
#include "game/render_internal.h"

static u8 *QueueSpritePacket(GameOrderingTableEntry *ot, u8 *packetCursor,
                             s32 x, s32 y, s32 width, s32 height,
                             s32 textureU, s32 textureV, s32 clutIndex,
                             s32 intensity, s32 shadeTexture,
                             s32 semiTransparent) {
    SPRT *sprite = (SPRT *)packetCursor;

    SetSprt(sprite);
    SetShadeTex(sprite, shadeTexture);
    SetSemiTrans(sprite, semiTransparent);
    sprite->x0 = WrapRenderCoordinate16(x);
    sprite->y0 = WrapRenderCoordinate16(y);
    sprite->w = WrapRenderCoordinate16(width);
    sprite->h = WrapRenderCoordinate16(height);
    sprite->u0 = textureU;
    sprite->v0 = textureV;
    sprite->clut = clutIndex;
    if (!shadeTexture) {
        sprite->r0 = intensity;
        sprite->g0 = intensity;
        sprite->b0 = intensity;
    }
    AddPrim(ot, sprite);
    return (u8 *)(sprite + 1);
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *packetCursor, s32 x,
                    s32 y, s32 width, s32 height, s32 textureU,
                    s32 textureV, s32 clutIndex) {
    return QueueSpritePacket(ot, packetCursor, x, y, width, height, textureU,
                             textureV, clutIndex, 0, 1, 0);
}

u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *packetCursor,
                          s32 x, s32 y, s32 width, s32 height, s32 textureU,
                          s32 textureV, s32 clutIndex, s32 intensity) {
    return QueueSpritePacket(ot, packetCursor, x, y, width, height, textureU,
                             textureV, clutIndex, intensity, 0, 0);
}

u8 *GameQueueShadedSpriteTrans(GameOrderingTableEntry *ot, u8 *packetCursor,
                               s32 x, s32 y, s32 width, s32 height,
                               s32 textureU, s32 textureV, s32 clutIndex,
                               s32 intensity) {
    return QueueSpritePacket(ot, packetCursor, x, y, width, height, textureU,
                             textureV, clutIndex, intensity, 0, 1);
}

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *packetCursor, s32 x,
                         s32 y, s32 width, s32 height, s32 textureU,
                         s32 textureV, s32 clutIndex) {
    return QueueSpritePacket(ot, packetCursor, x, y, width, height, textureU,
                             textureV, clutIndex, 0, 1, 1);
}

u8 *GameQueueLine(GameOrderingTableEntry *ot, u8 *packetCursor, s32 x0,
                  s32 y0, s32 x1, s32 y1, s32 red, s32 green, s32 blue) {
    LINE_F2 *line = (LINE_F2 *)packetCursor;

    SetLineF2(line);
    line->x0 = WrapRenderCoordinate16(x0);
    line->y0 = WrapRenderCoordinate16(y0);
    line->x1 = WrapRenderCoordinate16(x1);
    line->y1 = WrapRenderCoordinate16(y1);
    line->r0 = red;
    line->g0 = green;
    line->b0 = blue;
    AddPrim(ot, line);
    return (u8 *)(line + 1);
}

/* DR_MODE, 12 bytes: sets the texture page (and the blend mode packed into it)
 * for the primitives that follow, links it into the ordering table and returns
 * the advanced packet cursor. */
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packetCursor,
                      s32 tpage) {
    DrawPacket *packet = (DrawPacket *)packetCursor;

    SetDrawMode(packet, 0, 1, (u16)tpage, g_DrawModeEnv);
    AddPrim(ot, packet);
    return (u8 *)(packet + 1);
}

u8 *GameQueueShadedTexturedRect(GameOrderingTableEntry *ot, u8 *packetCursor,
                                s32 x, s32 y, s32 w, s32 h, s32 u, s32 v,
                                s32 clutIndex, s32 tpage, s32 intensity) {
    POLY_FT4 *packet = (POLY_FT4 *)packetCursor;
    s16 width = WrapRenderCoordinate16(w);
    s16 height = WrapRenderCoordinate16(h);
    u8 textureU = u;
    u8 textureV = v;

    SetPolyFT4(packet);
    if (width < 0) {
        width += 1;
        textureU -= width;
    }
    if (height < 0) {
        height += 1;
        textureV -= height;
    }

    packet->x0 = WrapRenderCoordinate16(x);
    packet->y0 = WrapRenderCoordinate16(y);
    packet->x1 = WrapRenderCoordinate16(
        (int64_t)x + (width < 0 ? -width : width));
    packet->y1 = WrapRenderCoordinate16(y);
    packet->x2 = WrapRenderCoordinate16(x);
    packet->y2 = WrapRenderCoordinate16(
        (int64_t)y + (height < 0 ? -height : height));
    packet->x3 = packet->x1;
    packet->y3 = packet->y2;
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
    AddPrim(ot, packet);
    return (u8 *)(packet + 1);
}

u8 *GameQueueTexturedRect(GameOrderingTableEntry *ot, u8 *packetCursor, s32 x,
                          s32 y, s32 w, s32 h, s32 u, s32 v, s32 uSpan,
                          s32 vSpan, s32 clutIndex, s32 tpage) {
    POLY_FT4 *packet = (POLY_FT4 *)packetCursor;
    s16 width = WrapRenderCoordinate16(w);
    s16 height = WrapRenderCoordinate16(h);
    u8 textureU = u;
    u8 textureV = v;

    SetPolyFT4(packet);
    SetShadeTex(packet, 1);

    if (width < 0) {
        textureU -= width + 1;
    }
    if (height < 0) {
        textureV -= height + 1;
    }

    packet->x0 = WrapRenderCoordinate16(x);
    packet->y0 = WrapRenderCoordinate16(y);
    packet->x1 = WrapRenderCoordinate16(
        (int64_t)x + (width < 0 ? -width : width));
    packet->y1 = WrapRenderCoordinate16(y);
    packet->x2 = WrapRenderCoordinate16(x);
    packet->y2 = WrapRenderCoordinate16(
        (int64_t)y + (height < 0 ? -height : height));
    packet->x3 = packet->x1;
    packet->y3 = packet->y2;
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
    AddPrim(ot, packet);
    return (u8 *)(packet + 1);
}
