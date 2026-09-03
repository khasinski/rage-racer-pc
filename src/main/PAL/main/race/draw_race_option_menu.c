#include "game/prim.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_hud_internal.h"
#include "game/state.h"

enum {
    RACE_OPTION_SELECTION_TOP = 0x68,
    RACE_OPTION_SELECTION_ROW_HEIGHT = 10,
    RACE_OPTION_DIM_PASSES = 2,
    RACE_OPTION_RETRY_DIGIT_COUNT = 6,
};

static s32 ClampRaceOptionCursor(s32 cursor, s32 grandPrixMode) {
    s32 lastOption = grandPrixMode != 0 ? 1 : 2;

    if (cursor < 0) {
        return 0;
    }
    return cursor < lastOption ? cursor : lastOption;
}

static s32 RaceOptionRetryDigit(void) {
    s32 retries =
        g_CourseProgress != NULL ? g_CourseProgress->retriesRemaining : 0;

    if (retries < 0) {
        return 0;
    }
    return retries < RACE_OPTION_RETRY_DIGIT_COUNT
               ? retries
               : RACE_OPTION_RETRY_DIGIT_COUNT - 1;
}

void DrawRaceOptionMenu(s32 cursorRow) {
    GameOrderingTableEntry *ot;
    u8 *next;
    u8 *packet;
    s32 selectionY;
    POLY_FT4 *quad;
    SPRT *marquee;
    RaceOptionMarqueeState marqueeState;
    RaceOptionPulseState pulseState;
    s32 pass;

    cursorRow = ClampRaceOptionCursor(cursorRow, g_GrandPrixMode);
    ot = GamePrimaryOrderingTable(0);
    marquee = RENDER_PRIM_CURSOR_AS(SPRT);
    SetSprt(marquee);
    SetShadeTex(marquee, 0);
    marquee->x0 = 0x8C;
    marquee->y0 = 0x5A;
    marquee->w = 0x28;
    marquee->h = 8;
    marquee->u0 = 0xD8;
    marquee->v0 = 0x38;
    marquee->clut = 0x7893;
    marqueeState = AdvanceRaceOptionMarquee(
        g_RaceOptionScroll0, g_RaceOptionScroll1, g_SceneTimer);
    marquee->r0 = marqueeState.brightness;
    marquee->g0 = marqueeState.brightness;
    marquee->b0 = marqueeState.brightness;
    AddPrim(ot, marquee);

    g_RaceOptionScroll0 = marqueeState.firstScroll;
    g_RaceOptionScroll1 = marqueeState.secondScroll;

    next = QueueDrawAreaPrim(ot, (DrawPacket *)(marquee + 1),
                             0, 0, 0x140, 0xF0);
    g_RenderState.packetCursor = next;
    DrawText8x8((g_RaceOptionScroll0 >> 2) + 0xA0, 0x8A,
                &g_RaceOptionMarquee[marqueeState.textFrame][0], 0x7811);
    DrawText8x8((g_RaceOptionScroll1 >> 2) + 0xA0, 0x8A,
                &g_RaceOptionMarquee[marqueeState.textFrame][20], 0x7811);

    next = QueueDrawAreaPrim(ot, RENDER_PRIM_CURSOR_AS(DrawPacket),
                             0x72, 0x8A, 0x5C, 0xC);
    packet = GameQueueSprite(
        ot, next, 0x88, 0x6A, 0x30, 8, 0xD0, 0x10, 0x7893);
    if (g_GrandPrixMode != 0) {
        packet = GameQueueSprite(
            ot, packet, 0x88, 0x74, 0x30, 8, 0xA0, 0x28, 0x7893);
        packet = GameQueueSprite(
            ot, packet, 0x84, 0x7E, 0x30, 8, 0xD0, 0x28, 0x7893);
        packet = GameQueueSprite(
            ot, packet, 0xB8, 0x7E, 8, 8,
            RaceOptionRetryDigit() * 8, 0, 0x78CC);
        packet = GameQueueSprite(
            ot, packet, 0x78, 0x7E, 8, 8, 0xD8, 8, 0x78CC);
        packet = GameQueueSprite(
            ot, packet, 0xC0, 0x7E, 8, 8, 0xE8, 8, 0x78CC);
    } else {
        packet = GameQueueSprite(
            ot, packet, 0x85, 0x74, 0x38, 8, 0xA0, 0x40, 0x7893);
        packet = GameQueueSprite(
            ot, packet, 0x90, 0x7E, 0x28, 8, 0xD8, 0x40, 0x7893);
    }

    selectionY = cursorRow * RACE_OPTION_SELECTION_ROW_HEIGHT +
                 RACE_OPTION_SELECTION_TOP;
    packet = AddTilePrim(
        ot, packet, 0x80, selectionY, 0x40, 1, 0xFF, 0xFF, 0);
    packet = AddTilePrim(
        ot, packet, 0x80, selectionY + 0xB, 0x40, 1, 0xFF, 0xFF, 0);
    packet = AddTilePrim(
        ot, packet, 0x80, selectionY, 1, 0xB, 0xFF, 0xFF, 0);
    packet = AddTilePrim(
        ot, packet, 0xBF, selectionY, 1, 0xB, 0xFF, 0xFF, 0);

    /* The original overlay deliberately applies the same translucent tile
     * twice to make the paused race dark enough behind the menu. */
    for (pass = 0; pass < RACE_OPTION_DIM_PASSES; pass++) {
        packet = GameQueueTileTrans(
            ot, packet, 0x70, 0x50, 0x60, 0x48, 8, 8, 8);
    }
    quad = (POLY_FT4 *)packet;

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

    g_RenderState.packetCursor = QueueDrawModePrim(ot, (u8 *)(quad + 1), 9);
}
