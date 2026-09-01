#include "game/prim.h"
#include "game/render_types.h"
#include "game/render_state.h"
#include "game/render.h"

void SetDrawClipRect(void *ot, s32 x, s32 y, s32 w, s32 h) {
    void *otReg;
    u8 **scratch;
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
    scratch = &RENDER_PRIM_CURSOR_AS(u8);
    hReg = h;
    packet = *scratch;

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
                    *scratch = packet;
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
    RenderBufferAddress cursor;
    POLY_F4 *prim;
    s32 semiReg;
    u32 flagsReg;
    s32 y1Reg;
    s32 x2Reg;
    s32 y2Reg;
    s32 x3Reg;
    s32 y3Reg;
    s32 rReg;
    u8 *oldPrim;
    s16 x0Local;
    s16 y0Local;
    s16 x1Local;
    u8 gLocal;
    u8 bLocal;

    prim = RENDER_PRIM_CURSOR_AS(POLY_F4);
    semiReg = semiTrans;
    flagsReg = flags;
    y1Reg = y1;
    x2Reg = x2;
    y2Reg = y2;
    x3Reg = x3;
    y3Reg = y3;
    rReg = r;
    x0Local = x0;
    y0Local = y0;
    x1Local = x1;
    bLocal = b;
    
    gLocal = g;

    SetPolyF4(prim);
    SetSemiTrans(prim, semiReg);

    prim->x0 = x0Local;
    prim->y0 = y0Local;
    prim->x1 = x1Local;
    prim->y1 = y1Reg;
    prim->x2 = x2Reg;
    prim->y2 = y2Reg;
    prim->x3 = x3Reg;
    prim->y3 = y3Reg;
    prim->r0 = rReg;
    prim->g0 = gLocal;
    prim->b0 = bLocal;

    cursor.polyF4 = prim;
    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    semiReg = flagsReg;
    flagsReg &= 0x80;
    if (flagsReg == 0) {
        cursor.polyF4 = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, semiReg & 0xFFFF);
        prim = cursor.polyF4;
    }

    RENDER_PRIM_CURSOR_AS(POLY_F4) = prim;
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
    RenderBufferAddress cursor;
    s32 x0Reg;
    s32 y0Reg;
    s32 x1Reg;
    s32 y1Reg;
    s32 rReg;
    s32 gReg;
    s32 bReg;
    u8 alphaValue;
    u8 *a0Reg;
    TILE *prim;
    u8 *oldPrim;

    prim = RENDER_PRIM_CURSOR_AS(TILE);
    y1Reg = (u16)y1;
    rReg = (u8)r;
    gReg = (u8)g;
    bReg = (u8)b;
    alphaValue = (u8)alpha;
    x0Reg = x0;
    y0Reg = y0;
    x1Reg = x1;
    

    SetTile(prim);
    cursor.tile = prim;
    a0Reg = cursor.bytes;
    SetSemiTrans(a0Reg, alphaValue != 0xFF);

    prim->x0 = x0Reg;
    prim->y0 = y0Reg;
    prim->w = x1Reg;
    prim->h = y1Reg;
    prim->r0 = rReg;
    prim->g0 = gReg;
    prim->b0 = bReg;

    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    if (alphaValue != 0xFF) {
        cursor.tile = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, alphaValue);
        prim = cursor.tile;
    }

    RENDER_PRIM_CURSOR_AS(TILE) = prim;
}


void DrawLine(void *ot, s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g, s32 b, s32 alpha) {
    RenderBufferAddress cursor;
    s32 x0Reg;
    s32 y0Reg;
    s32 x1Reg;
    s32 y1Reg;
    s32 rReg;
    s32 gReg;
    s32 bReg;
    u8 alphaValue;
    u8 *a0Reg;
    LINE_F2 *prim;
    u8 *oldPrim;

    prim = RENDER_PRIM_CURSOR_AS(LINE_F2);
    y1Reg = (u16)y1;
    rReg = (u8)r;
    gReg = (u8)g;
    bReg = (u8)b;
    alphaValue = (u8)alpha;
    x0Reg = x0;
    y0Reg = y0;
    x1Reg = x1;
    

    SetLineF2(prim);
    cursor.lineF2 = prim;
    a0Reg = cursor.bytes;
    SetSemiTrans(a0Reg, alphaValue != 0xFF);

    prim->x0 = x0Reg;
    prim->y0 = y0Reg;
    prim->x1 = x1Reg;
    prim->y1 = y1Reg;
    prim->r0 = rReg;
    prim->g0 = gReg;
    prim->b0 = bReg;

    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    if (alphaValue != 0xFF) {
        cursor.lineF2 = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, alphaValue);
        prim = cursor.lineF2;
    }

    RENDER_PRIM_CURSOR_AS(LINE_F2) = prim;
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
    RenderBufferAddress cursor;
    s16 x0Reg;
    s16 y0Reg;
    s16 x1Reg;
    s32 y1Reg;
    s32 r0Reg;
    s32 g0Reg;
    s32 b0Reg;
    u8 alphaReg;
    u8 *a0Reg;
    LINE_G2 *prim;
    u8 *oldPrim;
    u8 r1Local;
    u8 g1Local;
    u8 b1Local;

    prim = RENDER_PRIM_CURSOR_AS(LINE_G2);
    y1Reg = y1;
    r0Reg = r0;
    g0Reg = g0;
    b0Reg = b0;
    
    r1Local = r1;
    alphaReg = alpha;
    x0Reg = x0;
    y0Reg = y0;
    x1Reg = x1;
    g1Local = g1;
    b1Local = b1;

    SetLineG2(prim);
    cursor.lineG2 = prim;
    a0Reg = cursor.bytes;
    SetSemiTrans(a0Reg, alphaReg != 0xFF);

    prim->x0 = x0Reg;
    prim->y0 = y0Reg;
    prim->x1 = x1Reg;
    prim->y1 = y1Reg;
    prim->r0 = r0Reg;
    prim->g0 = g0Reg;
    prim->b0 = b0Reg;
    prim->r1 = r1Local;
    prim->g1 = g1Local;
    prim->b1 = b1Local;

    oldPrim = cursor.bytes;
    prim++;
    AddPrim(ot, oldPrim);

    if (alphaReg != 0xFF) {
        cursor.lineG2 = prim;
        cursor.bytes = QueueDrawModePrim(ot, cursor.bytes, alphaReg);
        prim = cursor.lineG2;
    }

    RENDER_PRIM_CURSOR_AS(LINE_G2) = prim;
}

void DrawRectOutline(void *buf, s32 xa, s32 ya, s32 w, s32 h, u8 r, u8 g,
                     u8 b, u8 code) {
  s32 x_R19 = xa;
  s32 y_R18 = ya;
  s16 rowY;
  s32 x0;
  int lastColumn;
  s32 x1;
  s32 ytop2;
  s32 leftX;
  s32 ybot;
  s16 startX;
  s32 ybi;
  startX = (s16) x_R19;
  x0 = startX;
  leftX = x_R19;
  lastColumn = w - 1;
  x1 = (s16) (leftX + lastColumn);
  rowY = (s16) y_R18;
  h -= 1;
  DrawLine(buf, (s16)x0, (s16)rowY, (s16)x1, (s16)rowY, r, g, b, code);
  DrawLine(buf, (s16)x0, (s16)(y_R18 + 1), (s16)x1, (s16)(y_R18 + 1), r, g, b, code);
  ytop2 = (s16) (y_R18 + 2);
  ybot = y_R18 + h;
  ybi = (s16) (ybot - 2);
  DrawLine(buf, (s16)x0, (s16)ytop2, (s16)x0, (s16)ybi, r, g, b, code);
  DrawLine(buf, (s16)x1, (s16)ytop2, (s16)x1, (s16)ybi, r, g, b, code);
  DrawLine(buf, (s16)x0, (s16)ybot, (s16)x1, (s16)ybot, r, g, b, code);
  DrawLine(buf, (s16)x0, (s16)(ybot - 1), (s16)x1, (s16)(ybot - 1), r, g, b, code);
}
