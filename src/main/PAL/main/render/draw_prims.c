#include "game/prim.h"
#include "game/render_types.h"
#include "game/render_state.h"
#include "game/render.h"

void SetDrawClipRect(void *ot, s32 x, s32 y, s32 w, s32 h) {
    void *otReg;
    u8 **cursorSlot;
    u8 *packet;
    s16 xReg;
    s16 yReg;
    s16 wReg;
    s16 hReg;
    u8 *oldPacket;
    RenderBufferAddress packetAddress;
    s32 tmp;
    Rect rect;

    otReg = ot;
    xReg = x;
    yReg = y;
    wReg = w;
    cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);
    hReg = h;
    packet = *cursorSlot;

    if ((s16)x + (s16)w > 0) {
        if ((s16)xReg < 0) {
            wReg = w + x;
            xReg = 0;
        }

        if ((s16)xReg < 0x140) {
            if ((s16)xReg + (s16)wReg >= 0x140) {
                tmp = 0x140;
                wReg = tmp - xReg;
            }

            if ((s16)y + (s16)h > 0) {
                if ((s16)yReg < 0) {
                    hReg = h + y;
                    yReg = 0;
                }

                if ((s16)yReg < 0x1E0) {
                    if ((s16)yReg + (s16)hReg >= 0x1E0) {
                        tmp = 0x1E0;
                        hReg = tmp - yReg;
                    }

                    rect.x = xReg;
                    rect.y = yReg;
                    rect.w = wReg;
                    rect.h = hReg;
                    packetAddress.bytes = packet;
                    SetDrawArea(packetAddress.drawPacket, &rect);
                    oldPacket = packet;
                    packet += sizeof(DrawPacket);
                    AddPrim(otReg, oldPacket);
                    *cursorSlot = packet;
                }
            }
        }
    }
}


void DrawSprite(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0,
                u8 r, u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans,
                u32 flags) {
    RenderBufferAddress cursor;
    SPRT *prim;
    s32 clutReg;
    u8 *oldPrim;
    s32 div;
    s32 base;

    prim = RENDER_PRIM_CURSOR_AS(SPRT);
    SetSprt(prim);

    clutReg = clutX;
    SetShadeTex(prim, shadeTex);
    SetSemiTrans(prim, semiTrans);

    prim->x0 = x0;
    prim->y0 = y0;
    prim->w = x1;
    prim->h = y1;
    prim->u0 = u0;
    prim->v0 = v0;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;

    div = (clutReg & 0xFFFF) / 20U;
    clutReg &= 0xFFFF;
    base = (div + 0x1E0) << 6;
    prim->clut = base + (clutReg - (div * 20));

    cursor.sprite = prim;
    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    clutReg = flags;
    flags &= 0x80;
    if (flags == 0) {
        cursor.sprite = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, clutReg & 0xFFFF);
        prim = cursor.sprite;
    }

    RENDER_PRIM_CURSOR_AS(SPRT) = prim;
}


void DrawFlatTriangle(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    RenderBufferAddress cursor;
    POLY_F3 *prim;
    u8 *oldPrim;

    prim = RENDER_PRIM_CURSOR_AS(POLY_F3);
    SetPolyF3(prim);
    SetSemiTrans(prim, semiTrans);

    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = x2;
    prim->y2 = y2;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;

    cursor.polyF3 = prim;
    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    if ((flags & 0x80) == 0) {
        cursor.polyF3 = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, flags & 0xFFFF);
        prim = cursor.polyF3;
    }

    RENDER_PRIM_CURSOR_AS(POLY_F3) = prim;
}


void DrawFlatQuad(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2, u16 y2,
                  u16 x3, u16 y3, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    POLY_F4 *prim = RENDER_PRIM_CURSOR_AS(POLY_F4);
    u8 *next;

    SetPolyF4(prim);
    SetSemiTrans(prim, semiTrans);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = x2;
    prim->y2 = y2;
    prim->x3 = x3;
    prim->y3 = y3;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if ((flags & 0x80) == 0) {
        next = QueueDrawModePrim(ot, next, flags & 0xFFFF);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}

/*
 * Packs a POLY_FT4 (textured quad) at the render state's cursor and links it into
 * the ordering table. clutIndex is a linear palette slot turned into VRAM clut
 * coordinates: 20 cluts per row starting at y = 0x1E0.
 */
void GameDrawTexturedQuad(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage) {
    RenderBufferAddress cursor;
    POLY_FT4 *prim = RENDER_PRIM_CURSOR_AS(POLY_FT4);
    u32 d;
    u32 clutRow;
    u32 rem;
    s32 clut;
    u8 *oldPrim;

    SetPolyFT4(prim);
    SetShadeTex(prim, shadeTex);
    SetSemiTrans(prim, semiTrans);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = x2;
    prim->y2 = y2;
    prim->x3 = x3;
    prim->y3 = y3;
    prim->u0 = u0;
    prim->v0 = v0;
    prim->u1 = u1;
    prim->v1 = v1;
    prim->u2 = u2;
    prim->v2 = v2;
    prim->u3 = u3;
    prim->v3 = v3;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    prim->tpage = tpage;
    d = clutIndex;
    clutRow = d / 20;
    clut = (clutRow + 0x1E0) << 6;
    rem = d - clutRow * 20;
    clut = clut + rem;
    prim->clut = clut;
    cursor.polyFT4 = prim;
    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);
    RENDER_PRIM_CURSOR_AS(POLY_FT4) = prim;
}


void DrawSolidRect(void *ot, s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g, s32 b, s32 alpha) {
    TILE *prim = RENDER_PRIM_CURSOR_AS(TILE);
    u8 alphaValue = (u8)alpha;
    u8 *next;

    SetTile(prim);
    SetSemiTrans(prim, alphaValue != 0xFF);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->w = x1;
    prim->h = y1;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (alphaValue != 0xFF) {
        next = QueueDrawModePrim(ot, next, alphaValue);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}


void DrawLine(void *ot, s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g, s32 b, s32 alpha) {
    LINE_F2 *prim = RENDER_PRIM_CURSOR_AS(LINE_F2);
    u8 alphaValue = (u8)alpha;
    u8 *next;

    SetLineF2(prim);
    SetSemiTrans(prim, alphaValue != 0xFF);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (alphaValue != 0xFF) {
        next = QueueDrawModePrim(ot, next, alphaValue);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}


void DrawPolyLine3(void *ot, s16 x0, s16 y0, s16 x1, s16 y1, s16 x2, s16 y2,
                   u8 r, u8 g, u8 b, u8 alpha) {
    RenderBufferAddress cursor;
    LINE_F3 *prim;
    u8 *oldPrim;

    prim = RENDER_PRIM_CURSOR_AS(LINE_F3);
    SetLineF3(prim);
    SetSemiTrans(prim, alpha != 0xFF);

    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = x2;
    prim->y2 = y2;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;

    cursor.lineF3 = prim;
    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    if (alpha != 0xFF) {
        cursor.lineF3 = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, alpha);
        prim = cursor.lineF3;
    }

    RENDER_PRIM_CURSOR_AS(LINE_F3) = prim;
}


void DrawGradientLine(void *ot, s32 x0, s32 y0, s32 x1, u16 y1, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1, u8 alpha) {
    LINE_G2 *prim = RENDER_PRIM_CURSOR_AS(LINE_G2);
    u8 *next;

    SetLineG2(prim);
    SetSemiTrans(prim, alpha != 0xFF);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->r0 = r0;
    prim->g0 = g0;
    prim->b0 = b0;
    prim->r1 = r1;
    prim->g1 = g1;
    prim->b1 = b1;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (alpha != 0xFF) {
        next = QueueDrawModePrim(ot, next, alpha);
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}

void DrawRectOutline(void *buf, s32 xa, s32 ya, s32 w, s32 h, u8 r, u8 g,
                     u8 b, u8 code) {
    s16 left = xa;
    s16 right = xa + w - 1;
    s16 top = ya;
    s16 bottom = ya + h - 1;

    DrawLine(buf, left, top, right, top, r, g, b, code);
    DrawLine(buf, left, top + 1, right, top + 1, r, g, b, code);
    DrawLine(buf, left, top + 2, left, bottom - 2, r, g, b, code);
    DrawLine(buf, right, top + 2, right, bottom - 2, r, g, b, code);
    DrawLine(buf, left, bottom, right, bottom, r, g, b, code);
    DrawLine(buf, left, bottom - 1, right, bottom - 1, r, g, b, code);
}
