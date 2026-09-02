#include "game/prim.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_hud_internal.h"
#include "game/state.h"

void DrawRaceOptionMenu(s32 cursorRow) {
    GameOrderingTableEntry *ot;
    u8 *next;
    s32 selectionY;
    POLY_FT4 *quad;
    RenderBufferAddress prim;
    RaceOptionMarqueeState marqueeState;
    RaceOptionPulseState pulseState;

    ot = GamePrimaryOrderingTable(0);
    prim.bytes = RENDER_PRIM_CURSOR_AS(u8);
    SetSprt(prim.sprite);
    SetShadeTex(prim.sprite, 0);
    prim.sprite->x0 = 0x8C;
    prim.sprite->y0 = 0x5A;
    prim.sprite->w = 0x28;
    prim.sprite->h = 8;
    prim.sprite->u0 = 0xD8;
    prim.sprite->v0 = 0x38;
    prim.sprite->clut = 0x7893;
    marqueeState = AdvanceRaceOptionMarquee(
        g_RaceOptionScroll0, g_RaceOptionScroll1, g_SceneTimer);
    prim.sprite->r0 = marqueeState.brightness;
    prim.sprite->g0 = marqueeState.brightness;
    prim.sprite->b0 = marqueeState.brightness;
    AddPrim(ot, prim.sprite);
    prim.sprite++;

    g_RaceOptionScroll0 = marqueeState.firstScroll;
    g_RaceOptionScroll1 = marqueeState.secondScroll;

    next = QueueDrawAreaPrim(ot, prim.drawPacket, 0, 0, 0x140, 0xF0);
    g_RenderState.packetCursor = next;
    DrawText8x8((g_RaceOptionScroll0 >> 2) + 0xA0, 0x8A,
                &g_RaceOptionMarquee[marqueeState.textFrame][0], 0x7811);
    DrawText8x8((g_RaceOptionScroll1 >> 2) + 0xA0, 0x8A,
                &g_RaceOptionMarquee[marqueeState.textFrame][20], 0x7811);

    prim.bytes = RENDER_PRIM_CURSOR_AS(u8);
    next = QueueDrawAreaPrim(ot, prim.drawPacket, 0x72, 0x8A, 0x5C, 0xC);
    prim.bytes = GameQueueSprite(
        ot, next, 0x88, 0x6A, 0x30, 8, 0xD0, 0x10, 0x7893);
    if (g_GrandPrixMode != 0) {
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0x88, 0x74, 0x30, 8, 0xA0, 0x28, 0x7893);
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0x84, 0x7E, 0x30, 8, 0xD0, 0x28, 0x7893);
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0xB8, 0x7E, 8, 8,
            g_CourseProgress->retriesRemaining * 8, 0, 0x78CC);
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0x78, 0x7E, 8, 8, 0xD8, 8, 0x78CC);
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0xC0, 0x7E, 8, 8, 0xE8, 8, 0x78CC);
    } else {
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0x85, 0x74, 0x38, 8, 0xA0, 0x40, 0x7893);
        prim.bytes = GameQueueSprite(
            ot, prim.bytes, 0x90, 0x7E, 0x28, 8, 0xD8, 0x40, 0x7893);
    }

    selectionY = cursorRow * 10 + 0x68;
    prim.bytes = AddTilePrim(
        ot, prim.bytes, 0x80, selectionY, 0x40, 1, 0xFF, 0xFF, 0);
    prim.bytes = AddTilePrim(
        ot, prim.bytes, 0x80, selectionY + 0xB, 0x40, 1, 0xFF, 0xFF, 0);
    prim.bytes = AddTilePrim(
        ot, prim.bytes, 0x80, selectionY, 1, 0xB, 0xFF, 0xFF, 0);
    prim.bytes = AddTilePrim(
        ot, prim.bytes, 0xBF, selectionY, 1, 0xB, 0xFF, 0xFF, 0);

    prim.bytes = GameQueueTileTrans(
        ot, prim.bytes, 0x70, 0x50, 0x60, 0x48, 8, 8, 8);
    prim.bytes = GameQueueTileTrans(
        ot, prim.bytes, 0x70, 0x50, 0x60, 0x48, 8, 8, 8);
    quad = prim.polyFT4;

    pulseState = AdvanceRaceOptionPulse(g_RaceOptionPulseAngle);
    g_RaceOptionPulseAngle = pulseState.angle;

    SetPolyFT4(quad);
    quad->r0 = 0x60;
    quad->g0 = 0x60;
    quad->b0 = 0x60;
    quad->x0 = 0xA0 - pulseState.halfWidth;
    quad->x2 = 0xA0 - pulseState.halfWidth;
    quad->x1 = 0xA0 + pulseState.halfWidth;
    quad->x3 = 0xA0 + pulseState.halfWidth;
    quad->y0 = 0x58;
    quad->y1 = 0x58;
    quad->y2 = 0x90;
    quad->y3 = 0x90;
    quad->u0 = 0xA8;
    quad->v0 = 0xA8;
    quad->u1 = 0xFF;
    quad->v1 = 0xA8;
    quad->u2 = 0xA8;
    quad->v2 = 0xE0;
    quad->u3 = 0xFF;
    quad->v3 = 0xE0;
    quad->clut = 0x784B;
    quad->tpage = 9;
    AddPrim(ot, quad);

    prim.polyFT4++;
    g_RenderState.packetCursor = QueueDrawModePrim(ot, prim.bytes, 9);
}
