#ifndef GAME_PRIM_H
#define GAME_PRIM_H

#include "common.h"
#include "psyq/gpu.h"

u8 *QueueDrawModePrim(void *ot, u8 *prim, s32 tpage);
u8 *QueueDrawAreaPrim(void *ot, DrawPacket *packet, s16 x, s16 y, s32 width,
                      s32 height);
u8 *AddTilePrim(
    void *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 r,
    s32 g,
    s32 b);

#endif
