#include "game/prim.h"
#include "game/player_car_internal.h"
#include "game/render_internal.h"

void DrawLapNumber(void) {
    SPRT *packet = RENDER_PRIM_CURSOR_AS(SPRT);
    s32 divisor = 1;
    s32 digitIndex = 0;

    for (;;) {
        s32 quotient = g_PlayerCar.lap / divisor;
        SPRT *digit;

        if (quotient == 0 && digitIndex > 0) break;

        digit = packet++;
        SetSprt(digit);
        SetShadeTex(digit, 1);
        digit->u0 = (quotient % 10) * 24;
        digit->v0 = 0x48;
        digit->clut = 0x780B;
        digit->x0 = 0x120 - digitIndex * 0x18;
        digit->y0 = 0x10;
        digit->w = 0x18;
        digit->h = 0x20;
        AddPrim(GamePrimaryOrderingTable(0), digit);

        divisor *= 10;
        digitIndex++;
    }

    QueueDrawModePrim(GamePrimaryOrderingTable(0), (u8 *)packet, 9);
}
