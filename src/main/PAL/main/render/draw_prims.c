#include "game/prim.h"
#include "game/render_types.h"
#include "game/render_state.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdint.h>

enum {
    CLUTS_PER_VRAM_ROW = 20,
    CLUT_VRAM_START_Y = 0x1E0,
    DRAW_MODE_ALREADY_CONFIGURED = 0x80,
    DRAW_MODE_NONE = 0xFF,
};

static u16 LinearClutToVram(u16 index) {
    u16 row = index / CLUTS_PER_VRAM_ROW;

    return ((row + CLUT_VRAM_START_Y) << 6) +
           index % CLUTS_PER_VRAM_ROW;
}

static u8 *QueuePrimitiveDrawMode(GameOrderingTableEntry *ot, u8 *next,
                                  u32 flags) {
    if ((flags & DRAW_MODE_ALREADY_CONFIGURED) != 0) return next;

    return QueueDrawModePrim(ot, next, flags & UINT16_MAX);
}

void SetDrawClipRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 w, s32 h) {
    int64_t right = (int64_t)x + w;
    int64_t bottom = (int64_t)y + h;
    s32 left;
    s32 top;
    DrawPacket *packet;
    Rect rect;

    if (w <= 0 || h <= 0 || right <= 0 || bottom <= 0 ||
        x >= SCREEN_WIDTH || y >= 480) {
        return;
    }

    left = x < 0 ? 0 : x;
    top = y < 0 ? 0 : y;
    if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
    if (bottom > 480) bottom = 480;

    rect.x = left;
    rect.y = top;
    rect.w = right - left;
    rect.h = bottom - top;
    packet = RENDER_PRIM_CURSOR_AS(DrawPacket);
    SetDrawArea(packet, &rect);
    AddPrim(ot, packet);
    g_RenderState.packetCursor = packet + 1;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0,
                u8 r, u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans,
                u32 flags) {
    SPRT *prim = RENDER_PRIM_CURSOR_AS(SPRT);

    SetSprt(prim);
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
    prim->clut = LinearClutToVram(clutX);
    AddPrim(ot, prim);

    g_RenderState.packetCursor =
        QueuePrimitiveDrawMode(ot, (u8 *)(prim + 1), flags);
}

void DrawFlatTriangle(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    POLY_F3 *prim = RENDER_PRIM_CURSOR_AS(POLY_F3);

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
    AddPrim(ot, prim);

    g_RenderState.packetCursor =
        QueuePrimitiveDrawMode(ot, (u8 *)(prim + 1), flags);
}

void DrawFlatQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2, u16 y2,
                  u16 x3, u16 y3, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    POLY_F4 *prim = RENDER_PRIM_CURSOR_AS(POLY_F4);

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

    g_RenderState.packetCursor =
        QueuePrimitiveDrawMode(ot, (u8 *)(prim + 1), flags);
}

/*
 * Packs a POLY_FT4 (textured quad) at the render state's cursor and links it into
 * the ordering table. clutIndex is a linear palette slot turned into VRAM clut
 * coordinates: 20 cluts per row starting at y = 0x1E0.
 */
void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage) {
    POLY_FT4 *prim = RENDER_PRIM_CURSOR_AS(POLY_FT4);

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
    prim->clut = LinearClutToVram(clutIndex);
    AddPrim(ot, prim);
    g_RenderState.packetCursor = prim + 1;
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x0, s32 y0, s32 x1,
                   s32 y1, s32 r, s32 g, s32 b, s32 drawMode) {
    TILE *prim = RENDER_PRIM_CURSOR_AS(TILE);
    u8 drawModeValue = (u8)drawMode;
    u8 *next;

    SetTile(prim);
    SetSemiTrans(prim, drawModeValue != DRAW_MODE_NONE);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->w = x1;
    prim->h = y1;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (drawModeValue != DRAW_MODE_NONE) {
        next = QueueDrawModePrim(ot, next, drawModeValue);
    }
    g_RenderState.packetCursor = next;
}

void DrawLine(GameOrderingTableEntry *ot, s32 x0, s32 y0, s32 x1, s32 y1,
              s32 r, s32 g, s32 b, s32 drawMode) {
    LINE_F2 *prim = RENDER_PRIM_CURSOR_AS(LINE_F2);
    u8 drawModeValue = (u8)drawMode;
    u8 *next;

    SetLineF2(prim);
    SetSemiTrans(prim, drawModeValue != DRAW_MODE_NONE);
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (drawModeValue != DRAW_MODE_NONE) {
        next = QueueDrawModePrim(ot, next, drawModeValue);
    }
    g_RenderState.packetCursor = next;
}

void DrawPolyLine3(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, s16 y1, s16 x2, s16 y2,
                   u8 r, u8 g, u8 b, u8 drawMode) {
    LINE_F3 *prim = RENDER_PRIM_CURSOR_AS(LINE_F3);
    u8 *next;

    SetLineF3(prim);
    SetSemiTrans(prim, drawMode != DRAW_MODE_NONE);

    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = x2;
    prim->y2 = y2;
    prim->r0 = r;
    prim->g0 = g;
    prim->b0 = b;
    AddPrim(ot, prim);

    next = (u8 *)(prim + 1);
    if (drawMode != DRAW_MODE_NONE) {
        next = QueueDrawModePrim(ot, next, drawMode);
    }
    g_RenderState.packetCursor = next;
}

void DrawGradientLine(GameOrderingTableEntry *ot, s32 x0, s32 y0, s32 x1,
                      u16 y1, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1,
                      u8 b1, u8 drawMode) {
    LINE_G2 *prim = RENDER_PRIM_CURSOR_AS(LINE_G2);
    u8 *next;

    SetLineG2(prim);
    SetSemiTrans(prim, drawMode != DRAW_MODE_NONE);
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
    if (drawMode != DRAW_MODE_NONE) {
        next = QueueDrawModePrim(ot, next, drawMode);
    }
    g_RenderState.packetCursor = next;
}

void DrawRectOutline(GameOrderingTableEntry *ot, s32 xa, s32 ya, s32 w,
                     s32 h, u8 r, u8 g, u8 b, u8 code) {
    const s16 left = WrapSigned16(xa);
    const s16 right = WrapSigned16((int64_t)xa + w - 1);
    const s16 top = WrapSigned16(ya);
    const s16 bottom = WrapSigned16((int64_t)ya + h - 1);

    DrawLine(ot, left, top, right, top, r, g, b, code);
    DrawLine(ot, left, top + 1, right, top + 1, r, g, b, code);
    DrawLine(ot, left, top + 2, left, bottom - 2, r, g, b, code);
    DrawLine(ot, right, top + 2, right, bottom - 2, r, g, b, code);
    DrawLine(ot, left, bottom, right, bottom, r, g, b, code);
    DrawLine(ot, left, bottom - 1, right, bottom - 1, r, g, b, code);
}
