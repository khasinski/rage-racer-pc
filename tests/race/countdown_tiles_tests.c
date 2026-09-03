#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render.h"

#include <stdio.h>

RenderBufferAddress g_TileStripBuffers[2];
u8 g_TileStripStorage[2 * 512 * sizeof(TILE)];

static s32 s_drawSyncCalls;
static s32 s_setTileCalls;
static s32 s_termPrimCalls;
static s32 s_addPrimCalls;
static s32 s_badLink;

int DrawSync(int mode) {
    if (mode != 0) s_badLink = 1;
    s_drawSyncCalls++;
    return 0;
}

void SetTile(TILE *tile) {
    tile->code = 0x60;
    s_setTileCalls++;
}

void TermPrim(void *primitive) {
    ((TILE *)primitive)->tag = 0xFFFFFF;
    s_termPrimCalls++;
}

void AddPrim(void *previous, void *primitive) {
    if ((TILE *)primitive != (TILE *)previous + 1) s_badLink = 1;
    ((TILE *)primitive)->tag = ((TILE *)previous)->tag;
    ((TILE *)previous)->tag = (u_long)(uintptr_t)primitive;
    s_addPrimCalls++;
}

static int CheckTile(const TILE *tile, s32 row, s32 column) {
    return tile->code == 0x60 && tile->w == 2 && tile->h == 1 &&
           tile->x0 == 0xCD - column * 3 && tile->y0 == 0x5A + row * 2 &&
           tile->r0 == 0x20 && tile->g0 == 0x20 && tile->b0 == 0x20;
}

int main(void) {
    s32 buffer;
    s32 tile;

    BuildTileStrips();
    BuildTileStrips();
    if (g_TileStripBuffers[0].bytes != g_TileStripStorage ||
        g_TileStripBuffers[1].bytes != g_TileStripStorage + 512 * sizeof(TILE) ||
        s_drawSyncCalls != 2 || s_setTileCalls != 2048 ||
        s_termPrimCalls != 2048 || s_addPrimCalls != 2044 || s_badLink) {
        fprintf(stderr, "countdown tile setup or links are invalid\n");
        return 1;
    }

    for (buffer = 0; buffer < 2; buffer++) {
        TILE *tiles = g_TileStripBuffers[buffer].tile;

        for (tile = 0; tile < 511; tile++) {
            if (tiles[tile].tag !=
                (u_long)(uintptr_t)&tiles[tile + 1]) {
                fprintf(stderr, "countdown tile chain is invalid\n");
                return 1;
            }
        }
        if (tiles[511].tag != 0xFFFFFF) {
            fprintf(stderr, "countdown tile chain is not terminated\n");
            return 1;
        }
        if (!CheckTile(&tiles[0], 0, 0) ||
            !CheckTile(&tiles[31], 0, 31) ||
            !CheckTile(&tiles[32], 1, 0) ||
            !CheckTile(&tiles[511], 15, 31)) {
            fprintf(stderr, "countdown tile geometry is invalid\n");
            return 1;
        }
    }
    return 0;
}
