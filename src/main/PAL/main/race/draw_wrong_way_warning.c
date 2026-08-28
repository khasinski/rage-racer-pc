#include "game/diagnostics.h"
#include "game/prim.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "rage/hud_config.h"

/* The GPU packet cursor: scratchpad word 0. Every emitter here packs its
 * primitive at this address and bumps it past what it wrote. */
#define SCRATCH (SCRATCH_PRIM_CURSOR_AS(u8))

typedef union TachometerColorAddress {
    u8 *components;
    s32 *packed;
} TachometerColorAddress;

void DrawWrongWayWarning(void) {
    SPRT *packet;
    SPRT *next;
    s32 i;
    s32 x;
    s32 u;
    OT_TYPE *ot;
    SPRT *oldPacket;
    s32 temp;
    s32 uvOffset;
    u8 *ret;
    RenderBufferAddress nextAddress;

    next = SCRATCH_PRIM_CURSOR_AS(SPRT);
    i = 0;
    u = 0x48;
    x = 0x6C;
    packet = next;

    do {
        SetSprt(next);
        SetShadeTex(next, 1);

        temp = 0x78;
        packet->y0 = temp;
        
        uvOffset = (((i & 2) << 3) - (i & 2)) << 2;
        temp = -0x10 - uvOffset;
        uvOffset += 0x10;
        packet->u0 = temp;
        temp = 0x10;
        packet->h = temp;
        temp = 0x788C;
        oldPacket = packet;
        packet->x0 = x;
        packet->v0 = u;
        packet->w = uvOffset;
        packet->clut = temp;

        packet++;
        next++;
        u += 0x10;
        x += 0x10;
        ot = GamePrimaryOrderingTable(0);
        i++;
        AddPrim(ot, oldPacket);
    } while (i < 3);

    nextAddress.sprite = next;
    ret = GameQueueTileTrans(GamePrimaryOrderingTable(0), nextAddress.bytes, 0x64, 0x70, 0x78, 0x20, 8, 8, 8);
    SCRATCH = ret;
    SCRATCH = QueueDrawModePrim(GamePrimaryOrderingTable(0), ret, 9);
}


void DrawTachometer(s32 rpm, s32 flash, s32 type, s32 amt) {
    CarTachometerSpec *p = &g_CarSpec->tachometer;
    s32 cx = p->needleX;
    s32 cy = p->needleY;
    s32 b = p->angleMin;
    s32 angle = b + rpm * (p->angleMax - b) / 10000;
    s32 cos = rsin(angle);
    s32 sin = rcos(angle);
    POLY_F4 *prim = SCRATCH_PRIM_CURSOR_AS(POLY_F4);
    s16 *vp;
    s16 *pa;
    s16 *pb;
    s32 i;
    u8 code7;

    cx = HudRightX(cx);

    SetPolyF4(prim);

    vp = &prim->x0;
    i = 0;
    pb = &g_TachoNeedleQuad[0][1];
    pa = pb - 1;
    for (; i < 4; i++) {
        *vp++ = cx + (sin * pa[0] - cos * pb[0]) / 4096;
        *vp++ = cy + (cos * pa[0] + sin * pb[0]) / 4096;
        pb += 2;
        pa += 2;
    }

    code7 = prim->code;

    if (type == 1) {
        if (amt > 96) amt = 96;
        g_TachoFaceB = -128 - amt;
        g_TachoFaceG = -128 - amt;
        g_TachoFaceR = -128 - amt;
        prim->r0 = (amt * 32 + p->needleColor[0] * (96 - amt)) / 96;
        prim->g0 = (amt * 32 + p->needleColor[1] * (96 - amt)) / 96;
        prim->b0 = (amt * 32 + p->needleColor[2] * (96 - amt)) / 96;
    } else if (type == 3) {
        u16 *clutAddress;

        amt -= 32;
        if (amt < 0) amt = 0;
        g_TachoFaceB = amt + 32;
        g_TachoFaceG = amt + 32;
        g_TachoFaceR = amt + 32;
        prim->r0 = ((96 - amt) * 32 + p->needleColor[0] * amt) / 96;
        prim->g0 = ((96 - amt) * 32 + p->needleColor[1] * amt) / 96;
        prim->b0 = ((96 - amt) * 32 + p->needleColor[2] * amt) / 96;
        {
            GameFrameContextAddress frame;
            frame.bytes = g_DrawBuffer;
            clutAddress = &frame.context->layout.raceHud.tachometerFace.clut;
        }
        *clutAddress = 0x33A8;
    } else if (type == 2) {
        TachometerColorAddress packetColor;
        TachometerColorAddress needleColor;
        u16 *clutAddress;

        {
            GameFrameContextAddress frame;
            frame.bytes = g_DrawBuffer;
            clutAddress = &frame.context->layout.raceHud.tachometerFace.clut;
        }
        *clutAddress = 0x33E8;
        g_TachoFaceB = 0x80;
        g_TachoFaceG = 0x80;
        g_TachoFaceR = 0x80;
        packetColor.components = &prim->r0;
        needleColor.components = p->needleColorAlt;
        *packetColor.packed = *needleColor.packed;
    } else {
        TachometerColorAddress packetColor;
        TachometerColorAddress needleColor;
        u16 *clutAddress;
        u16 rv = 0x33A8;

        {
            GameFrameContextAddress frame;
            frame.bytes = g_DrawBuffer;
            clutAddress = &frame.context->layout.raceHud.tachometerFace.clut;
        }
        *clutAddress = rv;
        g_TachoFaceB = 0x80;
        g_TachoFaceG = 0x80;
        g_TachoFaceR = 0x80;
        packetColor.components = &prim->r0;
        needleColor.components = p->needleColor;
        *packetColor.packed = *needleColor.packed;
    }

    prim->code = code7;
    if (DiagnosticsEnabled("render.tachometer_trace")) {
        printf("tacho rpm=%d angle=%d color=%02x%02x%02x "
               "quad=%d,%d/%d,%d/%d,%d/%d,%d "
               "v=%d,%d/%d,%d/%d,%d/%d,%d\n",
               rpm, angle, prim->r0, prim->g0, prim->b0,
               g_TachoNeedleQuad[0][0], g_TachoNeedleQuad[0][1],
               g_TachoNeedleQuad[1][0], g_TachoNeedleQuad[1][1],
               g_TachoNeedleQuad[2][0], g_TachoNeedleQuad[2][1],
               g_TachoNeedleQuad[3][0], g_TachoNeedleQuad[3][1],
               prim->x0, prim->y0, prim->x1, prim->y1,
               prim->x2, prim->y2, prim->x3, prim->y3);
    }
    AddPrim(GamePrimaryOrderingTable(0), prim);
    prim++;
    {
        RenderBufferAddress cursor;
        cursor.polyF4 = prim;
        SCRATCH = cursor.bytes;
    }

    {
        s32 x = cx + p->gearDigitDX;
        s32 y = cy + p->gearDigitDY;
        /* Not SCRATCH: this read has to stay volatile. Spelling it as the
         * plain macro lets the cursor written above be reused instead of
         * reloaded, which changes the output. */
        u8 *q = DrawHudDigit(SCRATCH_PRIM_CURSOR_VOLATILE, x, y,
                             g_PlayerCar.drive.gear, g_HudGlyphClut);
        SCRATCH = q;
        DrawSpeedDigits(cx, cy, g_PlayerCar.speed * 160 / 1168);
    }

    {
        GameFrameContextAddress frame;
        frame.bytes = g_DrawBuffer;
        frame.context->layout.raceHud.tachometerFace.r0 = g_TachoFaceR;
    }
    {
        GameFrameContextAddress frame;
        frame.bytes = g_DrawBuffer;
        frame.context->layout.raceHud.tachometerFace.g0 = g_TachoFaceG;
    }
    {
        GameFrameContextAddress frame;
        frame.bytes = g_DrawBuffer;
        frame.context->layout.raceHud.tachometerFace.b0 = g_TachoFaceB;
    }

    {
        GameFrameContext *frame = GetGameFrameContext(g_DrawBuffer);
        OT_TYPE *ot = GamePrimaryOrderingTable(0);

        frame->layout.raceHud.tachometerFace.x0 =
            HudRightX(g_TachoNeedleSprite.x);

        AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[0]);
        AddPrim(ot, &frame->layout.raceHud.tachometerFace);
        AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[1]);
    }

    {
        TILE *q = SCRATCH_PRIM_CURSOR_AS(TILE);
        OT_TYPE *ot;
        TILE *tile;
        s32 v10;
        SetTile(q);
        tile = q;
        q->x0 = cx + p->shiftLightDX;
        v10 = cy + p->shiftLightDY;
        q->w = 0x10;
        q->h = 0x10;
        q->r0 = flash * 223 + 32;
        q->g0 = 0x20;
        q->b0 = 0x20;
        ot = GamePrimaryOrderingTable(0);
        q->y0 = v10;
        q++;
        AddPrim(ot, tile);
        {
            RenderBufferAddress cursor;
            cursor.tile = q;
            SCRATCH = cursor.bytes;
        }
    }
}

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    OT_TYPE *ot = GamePrimaryOrderingTable(0);
    TILE *packet;
    TILE *prim;
    s32 height;

    if (color < 0) {
        color = 0;
    } else if (color >= 0x100) {
        color = 0xFF;
    }

    packet = SCRATCH_PRIM_CURSOR_AS(TILE);
    SetTile(packet);
    SetSemiTrans(packet, 1);

    packet->w = 0x140;
    height = 0xF0;
    packet->x0 = 0;
    packet->y0 = 0;
    packet->h = height;
    packet->r0 = color;
    packet->g0 = color;
    packet->b0 = color;

    prim = packet;
    packet++;
    AddPrim(ot, prim);
    {
        RenderBufferAddress cursor;
        cursor.tile = packet;
        SCRATCH = QueueDrawModePrim(GamePrimaryOrderingTable(0), cursor.bytes, tpage);
    }
}

u8 *DrawHudDigit(u8 *prim, s32 x, s32 y, s32 digit, u16 clut) {
    RenderBufferAddress cursor;
    SPRT_8 *out;
    s32 xReg = x;
    s32 yReg = y;
    s32 codeReg;
    s32 clutReg;

    cursor.bytes = prim;
    out = cursor.sprite8;
    
    codeReg = digit;
    clutReg = clut;
    SetSprt8(out);
    SetShadeTex(out, 1);

    codeReg <<= 3;
    out->u0 = codeReg;
    out->v0 = 0x10;

    {
        OT_TYPE *ot = GamePrimaryOrderingTable(0);
        SPRT_8 *oldPrim = out;

        out->x0 = xReg;
        out->y0 = yReg;
        out->clut = clutReg;
        out++;
        AddPrim(ot, oldPrim);
    }

    {
        RenderBufferAddress cursor;
        cursor.sprite8 = out;
        return cursor.bytes;
    }
}
