#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"

enum {
    FULLSCREEN_WIDTH = 320,
    STANDARD_SCREEN_HEIGHT = 240,
    INTERLACED_SCREEN_HEIGHT = 480,
};

static void DrawFullscreenFadeTileHeight(s32 color, s32 tpage, s32 height) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    TILE *tile = RENDER_PRIM_CURSOR_AS(TILE);

    if (color < 0) {
        color = 0;
    } else if (color > 0xFF) {
        color = 0xFF;
    }

    SetTile(tile);
    SetSemiTrans(tile, 1);
    tile->x0 = 0;
    tile->y0 = 0;
    tile->w = FULLSCREEN_WIDTH;
    tile->h = height;
    tile->r0 = color;
    tile->g0 = color;
    tile->b0 = color;

    AddPrim(ot, tile);
    g_RenderState.packetCursor =
        QueueDrawModePrim(ot, (u8 *)(tile + 1), tpage);
}

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    DrawFullscreenFadeTileHeight(color, tpage, STANDARD_SCREEN_HEIGHT);
}

void DrawFullscreenFadeTile480(s32 color, s32 tpage) {
    DrawFullscreenFadeTileHeight(color, tpage, INTERLACED_SCREEN_HEIGHT);
}
