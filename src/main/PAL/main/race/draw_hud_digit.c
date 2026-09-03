#include "game/prim.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render_internal.h"

u8 *DrawHudDigit(u8 *packet, s32 x, s32 y, s32 digit, u16 clut) {
    SPRT_8 *sprite = (SPRT_8 *)packet;

    if (digit < 0) {
        digit = 0;
    } else if (digit > 9) {
        digit = 9;
    }

    SetSprt8(sprite);
    SetShadeTex(sprite, 1);
    sprite->x0 = WrapRenderCoordinate16(x);
    sprite->y0 = WrapRenderCoordinate16(y);
    sprite->u0 = digit << 3;
    sprite->v0 = 0x10;
    sprite->clut = clut;

    AddPrim(GamePrimaryOrderingTable(0), sprite);
    return (u8 *)(sprite + 1);
}
