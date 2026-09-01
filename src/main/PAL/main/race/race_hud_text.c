#include "game/prim.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

typedef union CountdownPhase {
    s32 value;
    u32 unsignedValue;
} CountdownPhase;

void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor) {
    s32 whole;
    s32 fraction;
    s32 minutes;
    s32 seconds;
    s32 secondTens;
    s32 fractionHundreds;
    s32 fractionTens;
    s32 remainder;

    if (value >= 0) {
        whole = value / divisor;
        remainder = value % divisor;

        minutes = whole / 60;
        fraction = (remainder * 1000) / divisor;
        seconds = whole % 60;
        g_TimeTextBuffer[0] = minutes + '0';

        secondTens = seconds / 10;
        g_TimeTextBuffer[2] = secondTens + '0';
        g_TimeTextBuffer[3] = (seconds - (secondTens * 10)) + '0';

        fractionHundreds = fraction / 100;
        g_TimeTextBuffer[5] = fractionHundreds + '0';

        fractionTens = fraction / 10;
        g_TimeTextBuffer[6] = (fractionTens - (fractionHundreds * 10)) + '0';
        g_TimeTextBuffer[7] = (fraction - (fractionTens * 10)) + '0';
    } else {
        g_TimeTextBuffer[0] = '-';
        g_TimeTextBuffer[2] = '-';
        g_TimeTextBuffer[3] = '-';
        g_TimeTextBuffer[5] = '-';
        g_TimeTextBuffer[6] = '-';
        g_TimeTextBuffer[7] = '-';
    }

    DrawText8x8(x, y, g_TimeTextBuffer, color);
}

void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color) {
    s32 sec;
    s32 tmp;
    s32 min;
    char *p;
    s32 tens;
    s32 tens2;

    sec = ticks / 25;
    tmp = sec / 60;
    min = tmp;
    tmp = sec - min * 60;
    if (min < 10) {
        g_ClockTextBuffer = ' ';
    } else {
        g_ClockTextBuffer = min / 10 + '0';
    }
    tens = min / 10;
    p = g_ClockTextMinUnits;
    tens2 = tmp / 10;
    *p = min - tens * 10 + '0';
    g_ClockTextSecTens = tens2 + '0';
    g_ClockTextSecUnits = tmp - tens2 * 10 + '0';
    DrawText8x8(x, y, p - 1, color);
}

/* Which of the two frame buffers is being drawn, 0 or 1; the main loop sets
 * it beside g_DrawBuffer. Here it turns into the 240-line y bias of the
 * drawing-area rect. */

u8 *QueueDrawAreaPrim(void *ot, DrawPacket *packet, s16 x, s16 y, s32 w, s32 h) {
    DrawPacket *oldPacket;
    RenderBufferAddress nextPacket;
    Rect rect;
    s32 offset;

    offset = (g_FrameParity * 15) << 4;
    rect.x = x;
    rect.y = y + offset;
    rect.w = w;
    rect.h = h;
    SetDrawArea(packet, &rect);
    oldPacket = packet;
    packet++;
    AddPrim(ot, oldPacket);
    nextPacket.drawPacket = packet;
    return nextPacket.bytes;
}

void BuildTileStrips(void) {
    s32 row;
    s32 col;
    s32 bufferIndex;

    g_TileStripBuffers[0].bytes = g_TileStripStorage;
    g_TileStripBuffers[1].bytes =
        g_TileStripStorage + 512 * sizeof(TILE);
    DrawSync(0);

    for (bufferIndex = 0; bufferIndex < 2; bufferIndex++) {
        TILE *tiles = g_TileStripBuffers[bufferIndex].tile;

        for (row = 0; row < 16; row++) {
            for (col = 0; col < 32; col++) {
                s32 linear = row * 32 + col;
                TILE *tile = &tiles[linear];

                SetTile(tile);
                tile->w = 2;
                tile->h = 1;
                tile->x0 = 0xCD - col * 3;
                tile->y0 = 0x5A + row * 2;
                tile->r0 = 0x20;
                tile->g0 = 0x20;
                tile->b0 = 0x20;

                if (linear > 0) {
                    AddPrim(tile - 1, tile);
                }
            }
        }
    }
}

void DrawStartCountdown(s32 sceneTimer) {
    s32 timer;
    CountdownPhase phase;
    s32 halfStep;
    s32 wipeStart;
    s32 row;
    s32 column;
    u32 pattern;
    s32 colorBank;
    s32 phaseIsNegative;
    s32 rowOffset;
    u32 *firstPattern;
    u32 *patternBeforeFirst;
    u32 *phasePattern;
    TILE *tiles;
    u8 *cursor;
    s32 rangeTimer;
    u8 *orderingTable;
    SPRT *sprite;
    u8 *backdrop;
    RenderBufferAddress packetAddress;

    timer = sceneTimer;
    orderingTable = (u8 *)GamePrimaryOrderingTable(1);
    if (timer < 105) {
        return;
    }
    rangeTimer = timer - 90;
    if (rangeTimer >= 210) {
        return;
    }

    phase.value = rangeTimer / 30;
    if (phase.value < 0) {
        phase.value = 0;
    } else if (phase.value >= 5) {
        phase.value = -1;
    }

    halfStep = (timer % 30) / 2;

    if (phase.value == 4 || phase.value < 0) {
        halfStep = (timer & 2) << 2;
    } else if (phase.value == 0) {
        halfStep = 0;
    } else {
        if (halfStep >= 8) {
            halfStep = 8;
        } else if (halfStep <= 0) {
            halfStep = 0;
        }
    }

    row = 0;
    phaseIsNegative = phase.value < row;
    wipeStart = 7 - halfStep;
    tiles = g_TileStripBuffers[g_FrameParity].tile;

    do {
        firstPattern = g_CountdownDigitPatterns;
        patternBeforeFirst = g_CountdownGlyphTable;
        if (phase.value <= 0) {
            phasePattern = firstPattern;
        } else if (phase.value < 4) {
            phasePattern = patternBeforeFirst + (phase.value * 16);
        } else {
            phasePattern = firstPattern + ((phase.value - 4) * 16);
        }
        if (phase.value == 0) {
            pattern = -1;
        } else if (phaseIsNegative) {
            pattern = firstPattern[row];
        } else {
            pattern = phasePattern[row];
        }
        column = 0;
        if (wipeStart < row) {
            if (row < halfStep + 8) {
                pattern = ~pattern;
            }
        }
        rowOffset = row * 32;
        do {
            TILE *tile = &tiles[rowOffset + column];
            colorBank = 0;
            if (phase.value == 4 || phaseIsNegative) {
                colorBank = 1;
            }
            {
                CVec *colors = &g_CountdownCellColors[colorBank * 2];
                *(CVec *)&tile->r0 = colors[pattern & 1];
            }
            pattern >>= 1;
            column++;
        } while (column < 32);
        row++;
    } while (row < 16);

    if (phase.value < 0) {
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

        if (phase.unsignedValue < 4) {
            if (phase.value - 1 == row % 3) {
                halfStep = timer % 30;
                if (halfStep < 16) {
                    pattern = halfStep * 8;
                } else {
                    pattern = 0x80;
                }
            } else {
                pattern = 0x80;
            }
            if (phase.value - 1 >= row % 3) {
                sprite->clut = 0x7851;
            } else {
                sprite->clut = 0x784F;
            }
        } else {
            if (phase.value == 4) {
                halfStep = timer % 30;
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
        {
            SPRT *currentSprite = sprite;

            cursor += sizeof(SPRT);
            sprite = (SPRT *)cursor;
            AddPrim(orderingTable, currentSprite);
        }
    }

    RENDER_PRIM_CURSOR_AS(u8) = cursor;
    cursor = QueueDrawModePrim(GamePrimaryOrderingTable(1), cursor, 0xC);
    RENDER_PRIM_CURSOR_AS(u8) = cursor;

    if (phase.value > 0) {
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
    s32 selectedRow = cursorRow;
    u8 *ot;
    u8 *firstNext;
    s32 brightness;
    s32 marquee;
    RenderBufferAddress prim;

    ot = (u8 *)GamePrimaryOrderingTable(0);
    {
        void *drawPrim;

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
        if (g_RaceOptionScroll0 & 0x10) {
            
            brightness = 0x80;
        } else {
            brightness = 0x40;
        }
        prim.sprite->r0 = brightness;
        prim.sprite->g0 = brightness;
        prim.sprite->b0 = brightness;
        
        drawPrim = prim.pointer;
        prim.bytes += sizeof(SPRT);
        AddPrim(ot, drawPrim);

        g_RaceOptionScroll0 -= 4;
        g_RaceOptionScroll1 -= 4;
        if ((g_RaceOptionScroll0 >> 2) < -0x9C) {
            g_RaceOptionScroll0 = 0xF0;
        }
        if ((g_RaceOptionScroll1 >> 2) < -0x9C) {
            g_RaceOptionScroll1 = 0xF0;
        }

        firstNext =
            QueueDrawAreaPrim(ot, prim.drawPacket, 0, 0, 0x140, 0xF0);
    }

    {
        s32 fontU;
        s16 scroll0;
        char *marqueeBase;
        u8 *drawPrim;

        {
            s32 textY = 0x8A;
            s32 textColor = 0x7811;

            
            RENDER_PRIM_CURSOR_AS(u8) = firstNext;
            scroll0 = g_RaceOptionScroll0;
            marqueeBase = &g_RaceOptionMarquee[0][0];
            marquee = (g_SceneTimer & 3) * 40;
            DrawText8x8(
                (scroll0 >> 2) + 0xA0,
                textY,
                &marqueeBase[marquee],
                textColor);
        }
        {
            s32 secondTextY = 0x8A;

            
            marqueeBase += 20;
            DrawText8x8(
                (g_RaceOptionScroll1 >> 2) + 0xA0,
                secondTextY,
                &marqueeBase[marquee],
                0x7811);
        }

        firstNext = RENDER_PRIM_CURSOR_AS(u8);
        drawPrim = QueueDrawAreaPrim(ot, (DrawPacket *)firstNext,
                                     0x72, 0x8A, 0x5C, 0xC);
        fontU = 0xD0;
        prim.bytes = GameQueueSprite(
            ot, drawPrim, 0x88, 0x6A, 0x30, 8, fontU, 0x10, 0x7893);
        if (g_GrandPrixMode != 0) {
            prim.bytes = GameQueueSprite(
                ot, prim.bytes, 0x88, 0x74, 0x30, 8, 0xA0, 0x28, 0x7893);
            prim.bytes = GameQueueSprite(
                ot, prim.bytes, 0x84, 0x7E, 0x30, 8, fontU, 0x28, 0x7893);
            prim.bytes = GameQueueSprite(
                ot,
                prim.bytes,
                0xB8,
                0x7E,
                8,
                8,
                g_CourseProgress->retriesRemaining * 8,
                0,
                0x78CC);
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

        {
            s32 y;

            y = selectedRow * 10 + 0x68;
            prim.bytes = AddTilePrim(
                ot, prim.bytes, 0x80, y, 0x40, 1, 0xFF, 0xFF, 0);
            prim.bytes = AddTilePrim(
                ot, prim.bytes, 0x80, y + 0xB, 0x40, 1, 0xFF, 0xFF, 0);
            prim.bytes = AddTilePrim(
                ot, prim.bytes, 0x80, y, 1, 0xB, 0xFF, 0xFF, 0);
            prim.bytes = AddTilePrim(
                ot, prim.bytes, 0xBF, y, 1, 0xB, 0xFF, 0xFF, 0);
        }

        prim.bytes =
            GameQueueTileTrans(ot, prim.bytes, 0x70, 0x50, 0x60, 0x48, 8, 8, 8);

        {
            POLY_FT4 *quadBase;
            POLY_FT4 *quad;
            RenderBufferAddress quadAddress;

            quadAddress.bytes = GameQueueTileTrans(
                ot, prim.bytes, 0x70, 0x50, 0x60, 0x48, 8, 8, 8);
            quadBase = quadAddress.polyFT4;
            {
                s32 leftTrig;
                s16 left;

                quad = quadBase;
                g_RaceOptionPulseAngle += 0x20;
                SetPolyFT4(quad);
                quad->r0 = 0x60;
                quad->g0 = 0x60;
                quad->b0 = 0x60;
                g_RaceOptionPulseAngle &= 0xFFF;
                leftTrig = rcos(g_RaceOptionPulseAngle) * 0x2C;
                if (leftTrig < 0) {
                    leftTrig += 0xFFF;
                }
                left = 0xA0 - (leftTrig >> 12);
                quad->x2 = left;
                quad->x0 = left;
                quad->y1 = 0x58;
                quad->y0 = 0x58;
            }
            {
                POLY_FT4 *drawPrim;
                s32 rightTrig;
                s32 sample;
                s16 right;
                RenderBufferAddress cursor;

                g_RaceOptionPulseAngle &= 0xFFF;
                sample = rcos(g_RaceOptionPulseAngle);
                
                rightTrig = sample * 0x2C;
                quad = quadBase + 1;
                if (rightTrig < 0) {
                    rightTrig += 0xFFF;
                }
                right = (rightTrig >> 12) + 0xA0;
                drawPrim = quadBase;
                drawPrim->x3 = right;
                drawPrim->x1 = right;
                drawPrim->y3 = 0x90;
                drawPrim->y2 = 0x90;
                drawPrim->u0 = 0xA8;
                drawPrim->v0 = 0xA8;
                drawPrim->u1 = 0xFF;
                drawPrim->v1 = 0xA8;
                drawPrim->u2 = 0xA8;
                drawPrim->v2 = 0xE0;
                drawPrim->u3 = 0xFF;
                drawPrim->v3 = 0xE0;
                drawPrim->clut = 0x784B;
                drawPrim->tpage = 9;
                AddPrim(GamePrimaryOrderingTable(0), drawPrim);

                cursor.polyFT4 = quad;
                RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(
                    GamePrimaryOrderingTable(0), cursor.bytes, 9);
            }
        }
    }
}
