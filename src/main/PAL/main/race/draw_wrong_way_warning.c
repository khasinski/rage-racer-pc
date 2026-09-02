#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

typedef struct WrongWaySpriteLayout {
    u8 x;
    u8 u;
    u8 v;
    u8 width;
} WrongWaySpriteLayout;

static const WrongWaySpriteLayout s_WrongWaySprites[] = {
    {0x6C, 0xF0, 0x48, 0x10},
    {0x7C, 0xF0, 0x58, 0x10},
    {0x8C, 0xB8, 0x68, 0x48},
};

enum { WRONG_WAY_SPRITE_COUNT = 3 };

void DrawWrongWayWarning(void) {
    SPRT *sprites = RENDER_PRIM_CURSOR_AS(SPRT);
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 i;
    u8 *ret;

    for (i = 0; i < WRONG_WAY_SPRITE_COUNT; i++) {
        const WrongWaySpriteLayout *layout = &s_WrongWaySprites[i];
        SPRT *sprite = &sprites[i];

        SetSprt(sprite);
        SetShadeTex(sprite, 1);
        sprite->x0 = layout->x;
        sprite->y0 = 0x78;
        sprite->u0 = layout->u;
        sprite->v0 = layout->v;
        sprite->w = layout->width;
        sprite->h = 0x10;
        sprite->clut = 0x788C;
        AddPrim(ot, sprite);
    }

    ret = GameQueueTileTrans(ot, (u8 *)(sprites + WRONG_WAY_SPRITE_COUNT),
                             0x64, 0x70, 0x78, 0x20, 8, 8, 8);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, ret, 9);
}
