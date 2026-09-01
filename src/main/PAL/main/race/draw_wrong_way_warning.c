#include "game/diagnostics.h"
#include "game/prim.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "rage/hud_config.h"

/* The GPU packet cursor in the render state. Every emitter here packs its
 * primitive at this address and bumps it past what it wrote. */

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

    next = RENDER_PRIM_CURSOR_AS(SPRT);
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

    ret = GameQueueTileTrans(GamePrimaryOrderingTable(0), (u8 *)next,
                             0x64, 0x70, 0x78, 0x20, 8, 8, 8);
    RENDER_PRIM_CURSOR_AS(u8) = ret;
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(GamePrimaryOrderingTable(0), ret, 9);
}


void DrawTachometer(s32 rpm, s32 flash, s32 type, s32 amt) {
    CarTachometerSpec *p = &g_CarSpec->tachometer;
    s32 cx = p->needleX;
    s32 cy = p->needleY;
    s32 b = p->angleMin;
    s32 angle = b + rpm * (p->angleMax - b) / 10000;
    s32 cos = rsin(angle);
    s32 sin = rcos(angle);
    POLY_F4 *prim = RENDER_PRIM_CURSOR_AS(POLY_F4);
    s16 *vp;
    s16 *pa;
    s16 *pb;
    s32 i;

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

    if (type == 1) {
        if (amt > 96) amt = 96;
        g_TachoFaceB = -128 - amt;
        g_TachoFaceG = -128 - amt;
        g_TachoFaceR = -128 - amt;
        prim->r0 = (amt * 32 + p->needleColor[0] * (96 - amt)) / 96;
        prim->g0 = (amt * 32 + p->needleColor[1] * (96 - amt)) / 96;
        prim->b0 = (amt * 32 + p->needleColor[2] * (96 - amt)) / 96;
    } else if (type == 3) {
        amt -= 32;
        if (amt < 0) amt = 0;
        g_TachoFaceB = amt + 32;
        g_TachoFaceG = amt + 32;
        g_TachoFaceR = amt + 32;
        prim->r0 = ((96 - amt) * 32 + p->needleColor[0] * amt) / 96;
        prim->g0 = ((96 - amt) * 32 + p->needleColor[1] * amt) / 96;
        prim->b0 = ((96 - amt) * 32 + p->needleColor[2] * amt) / 96;
        GetGameFrameContext(g_DrawBuffer)->layout.raceHud.tachometerFace.clut =
            0x33A8;
    } else if (type == 2) {
        GetGameFrameContext(g_DrawBuffer)->layout.raceHud.tachometerFace.clut =
            0x33E8;
        g_TachoFaceB = 0x80;
        g_TachoFaceG = 0x80;
        g_TachoFaceR = 0x80;
        prim->r0 = p->needleColorAlt[0];
        prim->g0 = p->needleColorAlt[1];
        prim->b0 = p->needleColorAlt[2];
    } else {
        GetGameFrameContext(g_DrawBuffer)->layout.raceHud.tachometerFace.clut =
            0x33A8;
        g_TachoFaceB = 0x80;
        g_TachoFaceG = 0x80;
        g_TachoFaceR = 0x80;
        prim->r0 = p->needleColor[0];
        prim->g0 = p->needleColor[1];
        prim->b0 = p->needleColor[2];
    }

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
    RENDER_PRIM_CURSOR_AS(u8) = (u8 *)prim;

    {
        s32 x = cx + p->gearDigitDX;
        s32 y = cy + p->gearDigitDY;
        /* Not RENDER_PRIM_CURSOR_AS(u8): this read has to stay volatile. Spelling it as the
         * plain macro lets the cursor written above be reused instead of
         * reloaded, which changes the output. */
        u8 *q = DrawHudDigit(RENDER_PRIM_CURSOR_VOLATILE, x, y,
                             g_PlayerCar.drive.gear, g_HudGlyphClut);
        RENDER_PRIM_CURSOR_AS(u8) = q;
        DrawSpeedDigits(cx, cy, g_PlayerCar.speed * 160 / 1168);
    }

    {
        GameFrameContext *frame = GetGameFrameContext(g_DrawBuffer);
        OT_TYPE *ot = GamePrimaryOrderingTable(0);

        frame->layout.raceHud.tachometerFace.r0 = g_TachoFaceR;
        frame->layout.raceHud.tachometerFace.g0 = g_TachoFaceG;
        frame->layout.raceHud.tachometerFace.b0 = g_TachoFaceB;
        frame->layout.raceHud.tachometerFace.x0 =
            HudRightX(g_TachoNeedleSprite.x);

        AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[0]);
        AddPrim(ot, &frame->layout.raceHud.tachometerFace);
        AddPrim(ot, &frame->layout.raceHud.tachometerDrawModes[1]);
    }

    {
        TILE *q = RENDER_PRIM_CURSOR_AS(TILE);
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
        RENDER_PRIM_CURSOR_AS(u8) = (u8 *)q;
    }
}

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    OT_TYPE *ot = GamePrimaryOrderingTable(0);
    TILE *packet;
    TILE *prim;

    if (color < 0) {
        color = 0;
    } else if (color >= 0x100) {
        color = 0xFF;
    }

    packet = RENDER_PRIM_CURSOR_AS(TILE);
    SetTile(packet);
    SetSemiTrans(packet, 1);

    packet->w = 0x140;
    packet->x0 = 0;
    packet->y0 = 0;
    packet->h = 0xF0;
    packet->r0 = color;
    packet->g0 = color;
    packet->b0 = color;

    prim = packet;
    packet++;
    AddPrim(ot, prim);
    RENDER_PRIM_CURSOR_AS(u8) =
        QueueDrawModePrim(GamePrimaryOrderingTable(0), (u8 *)packet, tpage);
}

u8 *DrawHudDigit(u8 *prim, s32 x, s32 y, s32 digit, u16 clut) {
    SPRT_8 *out;

    out = (SPRT_8 *)prim;
    
    SetSprt8(out);
    SetShadeTex(out, 1);

    out->u0 = digit << 3;
    out->v0 = 0x10;

    {
        OT_TYPE *ot = GamePrimaryOrderingTable(0);
        SPRT_8 *oldPrim = out;

        out->x0 = x;
        out->y0 = y;
        out->clut = clut;
        out++;
        AddPrim(ot, oldPrim);
    }

    return (u8 *)out;
}
