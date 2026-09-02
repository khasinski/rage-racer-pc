#include "game/prim.h"
#include "game/player_car_internal.h"
#include "game/render_internal.h"

void DrawLapNumber(void) {
    RenderBufferAddress packet = {.sprite = RENDER_PRIM_CURSOR_AS(SPRT)};
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 remaining = g_PlayerCar.lap;
    s32 digitIndex = 0;

    do {
        SPRT *digit;

        digit = packet.sprite++;
        SetSprt(digit);
        SetShadeTex(digit, 1);
        digit->u0 = (remaining % 10) * 24;
        digit->v0 = 0x48;
        digit->clut = 0x780B;
        digit->x0 = 0x120 - digitIndex * 0x18;
        digit->y0 = 0x10;
        digit->w = 0x18;
        digit->h = 0x20;
        AddPrim(ot, digit);

        remaining /= 10;
        digitIndex++;
    } while (remaining != 0);

    g_RenderState.packetCursor = QueueDrawModePrim(ot, packet.bytes, 9);
}
