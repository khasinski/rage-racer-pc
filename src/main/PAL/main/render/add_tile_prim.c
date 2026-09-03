#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_types.h"

static u8 *QueueTile(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                     s32 width, s32 height, s32 red, s32 green, s32 blue,
                     s32 semiTransparent) {
    TILE *tile = (TILE *)prim;

    SetTile(tile);
    SetSemiTrans(tile, semiTransparent);
    tile->x0 = WrapSigned16(x);
    tile->y0 = WrapSigned16(y);
    tile->w = WrapSigned16(width);
    tile->h = WrapSigned16(height);
    tile->r0 = red;
    tile->g0 = green;
    tile->b0 = blue;
    AddPrim(ot, tile);
    return (u8 *)(tile + 1);
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height, s32 red, s32 green, s32 blue) {
    return QueueTile(ot, prim, x, y, width, height, red, green, blue, 0);
}

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                       s32 width, s32 height, s32 red, s32 green, s32 blue) {
    return QueueTile(ot, prim, x, y, width, height, red, green, blue, 1);
}
