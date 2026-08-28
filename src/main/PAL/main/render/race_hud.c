#include "game/prim.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/render_internal.h"

#include "rage/hud_config.h"


void DrawRaceHudLabels(s32 mode) {
    s32 count;
    s32 i;
    GameSpriteDesc *descs;
    void **scratch;
    GameFrameContext *frame = GetGameFrameContext(g_DrawBuffer);
    OT_TYPE *ot = GamePrimaryOrderingTable(0);

    count = 9;
    if (mode != 0) {
        count = 0xC;
    }
    descs = mode != 0 ? g_RaceHudSpriteDescsGp
                      : g_RaceHudSpriteDescsTimeTrial;

    i = 6;
    if (i < count) {
        do {
            SPRT *prim = &frame->layout.raceHud.labels[i - 6];
            int label = i - 6;
            int visible = 1;
            prim->x0 = descs[i].x < 160 ? HudLeftX(descs[i].x)
                                        : HudRightX(descs[i].x);
            if (mode != 0) {
                if (label == 1 && !HudShowLapTimes()) visible = 0;
                if (label == 2 && !HudShowTimeLimit()) visible = 0;
            } else if (!HudShowLapTimes()) {
                visible = 0;
            }
            i++;
            if (!visible) continue;
            AddPrim(ot, prim);
        } while (i < count);
    }

    scratch = &SCRATCH_PRIM_CURSOR_AS(void);
    *scratch = QueueDrawModePrim(ot, *scratch, 9);
}

u8 *AddTilePrim(void *ot, u8 *prim, s32 x, s32 y, s32 w, s32 h, s32 r, s32 g, s32 b) {
    RenderBufferAddress cursor;
    TILE *tile;
    u8 *oldPrim;

    cursor.bytes = prim;
    tile = cursor.tile;
    SetTile(tile);

    oldPrim = prim;
    tile->x0 = x;
    tile->y0 = y;
    tile->w = w;
    tile->h = h;
    tile->r0 = r;
    tile->g0 = g;
    tile->b0 = b;

    prim += sizeof(*tile);
    AddPrim(ot, oldPrim);
    return prim;
}

/* Expands a GameSpriteDesc into a scratchpad SPRT. */
void BuildSpriteFromDesc(SPRT *prim, GameSpriteDesc *src) {
    SetSprt(prim);

    prim->x0 = src->x;
    prim->y0 = src->y;
    prim->w = src->w;
    prim->h = src->h;
    prim->u0 = src->u0;
    prim->v0 = src->v0;
    prim->clut = src->clut;
    SetSemiTrans(prim, src->semiTrans);
    SetShadeTex(prim, 1);
}


/* The lap-time column: one row per lap from the player timing table at x=0xFA,
 * y stepping 0xA, the current lap highlighted and unset laps drawn as -1. */
void DrawLapTimes(void) {
    s32 i;
    s32 visibleCount;
    s32 activeIndex;
    s32 tile;
    s32 y;
    s32 *valuePtr;
    GameFrameContext *frame;
    OT_TYPE *ot;
    s32 value;

    if (!HudShowLapTimes()) return;

    visibleCount = g_PlayerCar.lap;
    if (visibleCount > g_LapCount) {
        visibleCount = g_LapCount;
    }

    i = 0;
    /* Retail address 0x8009e836 is not an independent global: it is the
     * hudLapHighlightRow member inside g_PlayerCar.drive. */
    activeIndex = g_PlayerCar.drive.hudLapHighlightRow;
    if (g_LapCount > 0) {
        frame = GetGameFrameContext(g_DrawBuffer);
        ot = GamePrimaryOrderingTable(0);
        y = 0x2E;
        valuePtr = g_PlayerCar.lapTimes.table.milliseconds;

        do {
            if (i == activeIndex) {
                tile = 0x780F;
            } else if (valuePtr[0] > 0x927BE) {
                tile = 0x7890;
            } else {
                tile = 0x78CC;
            }

            if (i < visibleCount) {
                value = valuePtr[0];
            } else {
                value = -1;
            }

            DrawTimeValue(HudRightX(0xFA), y, value, tile, 0x3E8);
            frame->layout.raceHud.lapTimes[i].x0 =
                HudRightX((g_GrandPrixMode != 0
                    ? g_RaceHudSpriteDescsGp
                    : g_RaceHudSpriteDescsTimeTrial)[i].x);
            y += 0xA;
            valuePtr++;
            frame->layout.raceHud.lapTimes[i].clut = tile;
            AddPrim(ot, &frame->layout.raceHud.lapTimes[i]);
            i++;
        } while (i < g_LapCount);
    }

    DrawTimeValue(HudRightX(0xFA), 0x20, g_BestLapThisRace,
                  0x78CC, 0x3E8);
}

void DrawTimeRemaining(s32 time) {
    s32 clutIndex = 0x78CC;

    if (!HudShowTimeLimit()) return;

    if (time < 0x5DC) {
        clutIndex = 0x7811;
    }

    DrawMinuteSecondTime(HudLeftX(0xE), 0xD2, time, clutIndex);
}

/* The two race-position digits, from g_RacePosition; the tens digit is
 * blanked below 10 and the colour changes from 4th place down. */
void DrawRacePosition(void) {
    GameFrameContextAddress drawBuffer;
    u8 *base;
    s32 value;
    SPRT *left;
    SPRT *right;

    base = g_DrawBuffer;
    drawBuffer.bytes = base;
    value = g_RacePosition;
    left = &drawBuffer.context->layout.raceHud.labels[3];
    right = &drawBuffer.context->layout.raceHud.labels[4];

    if (value >= 10) {
        left->u0 = 0x18;
    } else {
        left->u0 = 0;
    }

    {
        s32 quotient;
        s32 digit;

        quotient = value / 10;
        digit = (value - quotient * 10) * 24;
        right->u0 = digit;
    }

    if (value < 4) {
        left->clut = 0x780B;
        right->clut = 0x780B;
    } else {
        left->clut = 0x780E;
        right->clut = 0x780E;
    }
}

void SetHudBlinkColor(s32 phase) {
    GameFrameContextAddress drawBuffer;

    drawBuffer.bytes = g_DrawBuffer;
    drawBuffer.context->layout.raceHud.labels[2].clut = phase ? 0x7811 : 0x7800;
}

void DrawSplitDelta(s32 delta, s32 y) {
    GameFrameContextAddress drawBuffer;
    SPRT *firstPrim;
    SPRT *secondPrim;
    s32 value;
    s32 temp;
    OT_TYPE *ot;

    value = delta * 8;
    drawBuffer.bytes = g_DrawBuffer;
    value += 0x50;
    firstPrim = &drawBuffer.context->layout.raceHud.labels[3];
    secondPrim = &drawBuffer.context->layout.raceHud.labels[4];
    ot = GamePrimaryOrderingTable(0);

    firstPrim->u0 = value;
    AddPrim(ot, firstPrim);

    if (y > 0) {
        secondPrim->u0 = 0x88;
        temp = 0x7810;
    } else if (y < 0) {
        secondPrim->u0 = 0x78;
        temp = 0x780F;
    } else {
        return;
    }

    secondPrim->clut = temp;
    AddPrim(ot, secondPrim);
}
