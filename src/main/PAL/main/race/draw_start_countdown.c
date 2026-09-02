#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_hud_internal.h"
#include "game/state.h"

enum {
    COUNTDOWN_TILE_COLUMNS = 32,
    COUNTDOWN_TILE_ROWS = 16,
    COUNTDOWN_TILE_COUNT = COUNTDOWN_TILE_COLUMNS * COUNTDOWN_TILE_ROWS,
    COUNTDOWN_LAMP_COUNT = 6,
};

void DrawStartCountdown(s32 sceneTimer) {
    s32 phase;
    s32 row;
    s32 column;
    TILE *tiles;
    GameOrderingTableEntry *orderingTable;
    u8 *backdrop;
    RenderBufferAddress packet;
    StartCountdownTiming timing;

    timing = CalculateStartCountdownTiming(sceneTimer);
    if (!timing.visible) {
        return;
    }

    orderingTable = GamePrimaryOrderingTable(1);
    phase = timing.phase;
    tiles = g_TileStripBuffers[g_FrameParity].tile;

    for (row = 0; row < COUNTDOWN_TILE_ROWS; row++) {
        StartCountdownRow countdownRow = BuildStartCountdownRow(
            phase, row, timing.wipeHalfStep, g_CountdownGlyphTable,
            g_CountdownDigitPatterns);
        u32 pattern = countdownRow.pattern;

        for (column = 0; column < COUNTDOWN_TILE_COLUMNS; column++) {
            TILE *tile = &tiles[row * COUNTDOWN_TILE_COLUMNS + column];
            CVec *colors =
                &g_CountdownCellColors[countdownRow.colorBank * 2];
            CVec color = colors[pattern & 1];

            tile->r0 = color.r;
            tile->g0 = color.g;
            tile->b0 = color.b;
            tile->code = color.cd;
            pattern >>= 1;
        }
    }

    g_CountdownBoardOffset =
        AdvanceStartCountdownBoard(phase, g_CountdownBoardOffset);

    packet.bytes = RENDER_PRIM_CURSOR_AS(u8);
    backdrop = QueueDrawModePrim(
        orderingTable, packet.bytes, 9);
    packet.bytes = GameQueueTexturePacketWide(
        orderingTable,
        GameQueueTexturePacketWide(
            orderingTable, backdrop, 0x70, g_CountdownBoardOffset + 66,
            0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
            GAME_TEXTURE_PACKET_SPRT),
        0x70, g_CountdownBoardOffset + 122,
        0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
        GAME_TEXTURE_PACKET_SPRT);

    for (row = 0; row < COUNTDOWN_LAMP_COUNT; row++) {
        StartCountdownLamp lamp =
            BuildStartCountdownLamp(phase, sceneTimer, row);
        SPRT *sprite = packet.sprite;

        SetSprt(sprite);
        sprite->w = 0x20;
        sprite->h = 0x18;
        sprite->u0 = 0xE0;
        sprite->v0 = 0xD0;
        sprite->x0 = (row % 3) * 32 + 112;
        sprite->y0 =
            (row / 3) * 56 + ((u16)g_CountdownBoardOffset + 66);

        sprite->clut = lamp.clut;
        sprite->r0 = lamp.intensity;
        sprite->g0 = lamp.intensity;
        sprite->b0 = lamp.intensity;
        AddPrim(orderingTable, sprite);
        packet.sprite++;
    }

    packet.bytes = QueueDrawModePrim(orderingTable, packet.bytes, 0xC);
    g_RenderState.packetCursor = packet.bytes;

    if (phase > 0 && g_RacePaused == 0) {
        AddPrims(orderingTable, tiles, tiles + COUNTDOWN_TILE_COUNT - 1);
    }

    tiles = packet.tile;
    SetTile(tiles);
    tiles->w = 0x64;
    tiles->h = 0x24;
    tiles->x0 = 0x6E;
    tiles->r0 = 5;
    tiles->g0 = 5;
    tiles->b0 = 5;
    tiles->y0 = (u16)g_CountdownBoardOffset + 88;
    AddPrim(orderingTable, tiles++);
    packet.tile = tiles;
    g_RenderState.packetCursor = packet.bytes;
}
