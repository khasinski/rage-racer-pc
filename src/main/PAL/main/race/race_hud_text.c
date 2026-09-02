#include "game/prim.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_hud_internal.h"
#include "game/state.h"

void DrawStartCountdown(s32 sceneTimer) {
    s32 phase;
    s32 halfStep;
    s32 wipeStart;
    s32 row;
    s32 column;
    u32 pattern;
    s32 phaseIsNegative;
    u32 *firstPattern;
    u32 *phasePattern;
    TILE *tiles;
    u8 *cursor;
    s32 rangeTimer;
    u8 *orderingTable;
    SPRT *sprite;
    u8 *backdrop;
    RenderBufferAddress packetAddress;
    StartCountdownTiming timing;

    timing = CalculateStartCountdownTiming(sceneTimer);
    if (!timing.visible) {
        return;
    }

    orderingTable = (u8 *)GamePrimaryOrderingTable(1);
    phase = timing.phase;
    halfStep = timing.wipeHalfStep;
    phaseIsNegative = phase < 0;
    wipeStart = 7 - halfStep;
    tiles = g_TileStripBuffers[g_FrameParity].tile;
    firstPattern = g_CountdownDigitPatterns;
    if (phase > 0 && phase < 4) {
        phasePattern = g_CountdownGlyphTable + phase * 16;
    } else {
        phasePattern = firstPattern;
    }

    for (row = 0; row < 16; row++) {
        s32 colorBank = phase == 4 || phaseIsNegative;

        if (phase == 0) {
            pattern = -1;
        } else if (phaseIsNegative) {
            pattern = firstPattern[row];
        } else {
            pattern = phasePattern[row];
        }
        if (wipeStart < row && row < halfStep + 8) {
            pattern = ~pattern;
        }
        for (column = 0; column < 32; column++) {
            TILE *tile = &tiles[row * 32 + column];
            CVec *colors = &g_CountdownCellColors[colorBank * 2];

            *(CVec *)&tile->r0 = colors[pattern & 1];
            pattern >>= 1;
        }
    }

    if (phase < 0) {
        g_CountdownBoardOffset -= 16;
        if (g_CountdownBoardOffset < -240) {
            g_CountdownBoardOffset = -240;
        }
    } else {
        g_CountdownBoardOffset = 0;
    }

    cursor = RENDER_PRIM_CURSOR_AS(u8);
    backdrop = QueueDrawModePrim(
        GamePrimaryOrderingTable(1), cursor, 9);
    pattern = g_CountdownBoardOffset;
    RENDER_PRIM_CURSOR_AS(u8) = backdrop;
    cursor = GameQueueTexturePacketWide(
        orderingTable,
        GameQueueTexturePacketWide(
            orderingTable, backdrop, 0x70, pattern + 66,
            0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
            GAME_TEXTURE_PACKET_SPRT),
        0x70, g_CountdownBoardOffset + 122,
        0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
        GAME_TEXTURE_PACKET_SPRT);

    packetAddress.bytes = cursor;
    sprite = packetAddress.sprite;
    for (row = 0; row < 6; row++) {
        SetSprt(cursor);
        sprite->w = 0x20;
        sprite->h = 0x18;
        sprite->u0 = 0xE0;
        sprite->v0 = 0xD0;
        sprite->x0 = (row % 3) * 32 + 112;
        sprite->y0 =
            (row / 3) * 56 + ((u16)g_CountdownBoardOffset + 66);

        if ((u32)phase < 4) {
            if (phase - 1 == row % 3) {
                halfStep = sceneTimer % 30;
                if (halfStep < 16) {
                    pattern = halfStep * 8;
                } else {
                    pattern = 0x80;
                }
            } else {
                pattern = 0x80;
            }
            if (phase - 1 >= row % 3) {
                sprite->clut = 0x7851;
            } else {
                sprite->clut = 0x784F;
            }
        } else {
            if (phase == 4) {
                halfStep = sceneTimer % 30;
                if (halfStep < 10) {
                    pattern = halfStep * 12;
                } else {
                    pattern = 0x80;
                }
            } else {
                pattern = 0x80;
            }
            sprite->clut = 0x7850;
        }

        sprite->r0 = pattern;
        sprite->g0 = pattern;
        sprite->b0 = pattern;
        AddPrim(orderingTable, sprite);
        cursor += sizeof(SPRT);
        sprite = (SPRT *)cursor;
    }

    RENDER_PRIM_CURSOR_AS(u8) = cursor;
    cursor = QueueDrawModePrim(GamePrimaryOrderingTable(1), cursor, 0xC);
    RENDER_PRIM_CURSOR_AS(u8) = cursor;

    if (phase > 0) {
        if (g_RacePaused == 0) {
            AddPrims(orderingTable, tiles, tiles + 511);
        }
    }

    tiles = RENDER_PRIM_CURSOR_AS(TILE);
    SetTile(tiles);
    rangeTimer = (u16)g_CountdownBoardOffset + 88;
    tiles->w = 0x64;
    tiles->h = 0x24;
    tiles->x0 = 0x6E;
    tiles->r0 = 5;
    tiles->g0 = 5;
    tiles->b0 = 5;
    tiles->y0 = rangeTimer;
    AddPrim(orderingTable, tiles++);
    RENDER_PRIM_CURSOR_AS(TILE) = tiles;
}


void DrawRaceOptionMenu(s32 cursorRow) {
    u8 *ot;
    u8 *next;
    s32 brightness;
    s32 marquee;
    s32 selectionY;
    s32 pulse;
    char *marqueeBase;
    POLY_FT4 *quad;
    RenderBufferAddress prim;
    RaceOptionMarqueeState marqueeState;
    RaceOptionPulseState pulseState;

    ot = (u8 *)GamePrimaryOrderingTable(0);
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
    brightness = marqueeState.brightness;
    prim.sprite->r0 = brightness;
    prim.sprite->g0 = brightness;
    prim.sprite->b0 = brightness;
    AddPrim(ot, prim.sprite);
    prim.bytes += sizeof(SPRT);

    g_RaceOptionScroll0 = marqueeState.firstScroll;
    g_RaceOptionScroll1 = marqueeState.secondScroll;

    next = QueueDrawAreaPrim(ot, prim.drawPacket, 0, 0, 0x140, 0xF0);
    RENDER_PRIM_CURSOR_AS(u8) = next;
    marqueeBase = &g_RaceOptionMarquee[0][0];
    marquee = marqueeState.textOffset;
    DrawText8x8((g_RaceOptionScroll0 >> 2) + 0xA0, 0x8A,
                &marqueeBase[marquee], 0x7811);
    DrawText8x8((g_RaceOptionScroll1 >> 2) + 0xA0, 0x8A,
                &marqueeBase[marquee + 20], 0x7811);

    next = QueueDrawAreaPrim(ot,
                             (DrawPacket *)RENDER_PRIM_CURSOR_AS(u8),
                             0x72, 0x8A, 0x5C, 0xC);
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
    pulse = pulseState.halfWidth;

    SetPolyFT4(quad);
    quad->r0 = 0x60;
    quad->g0 = 0x60;
    quad->b0 = 0x60;
    quad->x0 = 0xA0 - pulse;
    quad->x2 = 0xA0 - pulse;
    quad->x1 = 0xA0 + pulse;
    quad->x3 = 0xA0 + pulse;
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

    prim.polyFT4 = quad + 1;
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(ot, prim.bytes, 9);
}
