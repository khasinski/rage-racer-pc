#include "game/prim.h"
#include "game/render.h"

u8 *AddTilePrim(void *ot, u8 *prim, s32 x, s32 y, s32 width, s32 height,
                s32 red, s32 green, s32 blue) {
    TILE *tile = (TILE *)(void *)prim;

    SetTile(tile);
    tile->x0 = x;
    tile->y0 = y;
    tile->w = width;
    tile->h = height;
    tile->r0 = red;
    tile->g0 = green;
    tile->b0 = blue;
    AddPrim(ot, tile);
    return (u8 *)(tile + 1);
}
