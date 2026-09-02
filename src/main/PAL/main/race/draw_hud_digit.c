#include "game/prim.h"
#include "game/race.h"
#include "game/render_internal.h"

u8 *DrawHudDigit(u8 *packet, s32 x, s32 y, s32 digit, u16 clut) {
    RenderBufferAddress address = {.bytes = packet};
    SPRT_8 *sprite = address.sprite8;

    SetSprt8(sprite);
    SetShadeTex(sprite, 1);
    sprite->x0 = x;
    sprite->y0 = y;
    sprite->u0 = digit << 3;
    sprite->v0 = 0x10;
    sprite->clut = clut;

    AddPrim(GamePrimaryOrderingTable(0), sprite);
    address.sprite8++;
    return address.bytes;
}
