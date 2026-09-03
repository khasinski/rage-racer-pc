#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render.h"

enum {
    COUNTDOWN_TILE_COLUMNS = 32,
    COUNTDOWN_TILE_ROWS = 16,
    COUNTDOWN_TILES_PER_BUFFER = COUNTDOWN_TILE_COLUMNS * COUNTDOWN_TILE_ROWS,
};

void BuildTileStrips(void) {
    s32 bufferIndex;

    g_TileStripBuffers[0].bytes = g_TileStripStorage;
    g_TileStripBuffers[1].bytes =
        g_TileStripStorage + COUNTDOWN_TILES_PER_BUFFER * sizeof(TILE);
    DrawSync(0);

    for (bufferIndex = 0; bufferIndex < 2; bufferIndex++) {
        TILE *tiles = g_TileStripBuffers[bufferIndex].tile;
        s32 row;

        for (row = 0; row < COUNTDOWN_TILE_ROWS; row++) {
            s32 column;

            for (column = 0; column < COUNTDOWN_TILE_COLUMNS; column++) {
                s32 index = row * COUNTDOWN_TILE_COLUMNS + column;
                TILE *tile = &tiles[index];

                SetTile(tile);
                TermPrim(tile);
                tile->w = 2;
                tile->h = 1;
                tile->x0 = 0xCD - column * 3;
                tile->y0 = 0x5A + row * 2;
                tile->r0 = 0x20;
                tile->g0 = 0x20;
                tile->b0 = 0x20;

                if (index > 0) {
                    AddPrim(tile - 1, tile);
                }
            }
        }
    }
}
