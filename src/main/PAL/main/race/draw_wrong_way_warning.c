#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

void DrawWrongWayWarning(void) {
    SPRT *sprites = RENDER_PRIM_CURSOR_AS(SPRT);
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 i;
    u8 *ret;

    for (i = 0; i < 3; i++) {
        SPRT *sprite = &sprites[i];
        s32 uvOffset = (((i & 2) << 3) - (i & 2)) << 2;

        SetSprt(sprite);
        SetShadeTex(sprite, 1);
        sprite->x0 = 0x6C + i * 0x10;
        sprite->y0 = 0x78;
        sprite->u0 = (u8)(-0x10 - uvOffset);
        sprite->v0 = 0x48 + i * 0x10;
        sprite->w = uvOffset + 0x10;
        sprite->h = 0x10;
        sprite->clut = 0x788C;
        AddPrim(ot, sprite);
    }

    ret = GameQueueTileTrans(ot, (u8 *)(sprites + 3),
                             0x64, 0x70, 0x78, 0x20, 8, 8, 8);
    RENDER_PRIM_CURSOR_AS(u8) = ret;
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(ot, ret, 9);
}
