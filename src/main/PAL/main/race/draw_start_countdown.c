#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/race_hud_internal.h"
#include "game/state.h"

enum {
    COUNTDOWN_LAMP_COUNT = 6,
};

void DrawStartCountdown(s32 sceneTimer) {
    s32 phase;
    s32 row;
    s32 column;
    TILE *tiles;
    TILE *backdrop;
    GameOrderingTableEntry *orderingTable;
    u8 *packet;
    StartCountdownTiming timing;

    timing = CalculateStartCountdownTiming(sceneTimer);
    if (!timing.visible) {
        return;
    }

    orderingTable = GamePrimaryOrderingTable(1);
    phase = timing.phase;
    tiles = g_TileStripBuffers[
        CountdownTileBufferIndex(g_FrameParity)].tile;

    for (row = 0; row < START_COUNTDOWN_PATTERN_ROW_COUNT; row++) {
        StartCountdownRow countdownRow = BuildStartCountdownRow(
            phase, row, timing.wipeHalfStep, g_CountdownGlyphTable,
            g_CountdownDigitPatterns);
        const CVec *colors = g_CountdownCellColors[countdownRow.colorBank];
        u32 pattern = countdownRow.pattern;

        for (column = 0; column < START_COUNTDOWN_TILE_COLUMN_COUNT;
             column++) {
            TILE *tile =
                &tiles[row * START_COUNTDOWN_TILE_COLUMN_COUNT + column];
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

    packet = QueueDrawModePrim(
        orderingTable, RENDER_PRIM_CURSOR_AS(u8), 9);
    packet = GameQueueTexturePacketWide(
        orderingTable, packet, 0x70, g_CountdownBoardOffset + 66,
        0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
        GAME_TEXTURE_PACKET_SPRT);
    packet = GameQueueTexturePacketWide(
        orderingTable, packet, 0x70, g_CountdownBoardOffset + 122,
        0x60, 0x18, 0xA0, 0xE8, 0x60, 0x18, 0x784E, 9,
        GAME_TEXTURE_PACKET_SPRT);

    for (row = 0; row < COUNTDOWN_LAMP_COUNT; row++) {
        StartCountdownLamp lamp =
            BuildStartCountdownLamp(phase, sceneTimer, row);
        SPRT *sprite = (SPRT *)packet;

        SetSprt(sprite);
        sprite->w = 0x20;
        sprite->h = 0x18;
        sprite->u0 = 0xE0;
        sprite->v0 = 0xD0;
        sprite->x0 = (row % 3) * 32 + 112;
        sprite->y0 =
            (row / 3) * 56 + g_CountdownBoardOffset + 66;

        sprite->clut = lamp.clut;
        sprite->r0 = lamp.intensity;
        sprite->g0 = lamp.intensity;
        sprite->b0 = lamp.intensity;
        AddPrim(orderingTable, sprite);
        packet = (u8 *)(sprite + 1);
    }

    packet = QueueDrawModePrim(orderingTable, packet, 0xC);
    g_RenderState.packetCursor = packet;

    if (phase > 0 && g_RacePaused == 0) {
        AddPrims(orderingTable, tiles,
                 tiles + START_COUNTDOWN_TILES_PER_BUFFER - 1);
    }

    backdrop = (TILE *)packet;
    SetTile(backdrop);
    backdrop->w = 0x64;
    backdrop->h = 0x24;
    backdrop->x0 = 0x6E;
    backdrop->r0 = 5;
    backdrop->g0 = 5;
    backdrop->b0 = 5;
    backdrop->y0 = g_CountdownBoardOffset + 88;
    AddPrim(orderingTable, backdrop);
    g_RenderState.packetCursor = backdrop + 1;
}
