#include <sys/types.h>

#include "common.h"
#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_types.h"
#include "game/scratchpad.h"
#include "psyq/gpu.h"


void DrawSpriteString(long x, long y, u_char *str, long clutIndex) {
    volatile SPRT *packet;
    long idx;
    u_char *next;
    register u_char *sr;
    register u_char *tableA;
    long ga;
    long gb;
    long w;
    volatile SPRT *oldPacket;
    u_char *tableB;
    RenderBufferAddress packetAddress;

    sr = str;
    next = SCRATCH_PRIM_CURSOR_AS(u_char);
    if (*sr != 0) {
        tableA = g_SpriteFontU;
        tableB = g_SpriteFontV;
        packetAddress.bytes = next;
        packet = packetAddress.volatileSprite;
        do {
            idx = *sr++ - 0x20;
            if (idx != 0) {
                ga = tableA[idx * 2];
                gb = tableB[idx * 2];
                packetAddress.bytes = next;
                SetSprt(packetAddress.sprite);
                SetShadeTex(next, 1);
                packet = packetAddress.volatileSprite;
                oldPacket = packet;
                packet->x0 = x;
                packet->y0 = y;
                packet->u0 = ga;
                packet->v0 = gb;
                w = g_SpriteFontWidth[idx];
                packet->h = 0x18;
                packet->clut = clutIndex;
                packet->w = w;
                next += sizeof(SPRT);
                packetAddress.volatileSprite = oldPacket;
                AddPrim(GamePrimaryOrderingTable(0), packetAddress.sprite);
            }
            x += g_SpriteFontWidth[idx];
        } while (*sr != 0);
    }
    packetAddress.bytes = next;
    SetDrawMode(packetAddress.drawPacket, 0, 1, 0x1D, g_DrawModeEnv);
    AddPrim(GamePrimaryOrderingTable(0), next);
    SCRATCH_PRIM_CURSOR_AS(u_char) = next + sizeof(DrawPacket);
}

u8 *DrawShadowedTile(void *ot, u8 *prim, s32 x, s32 y) {
    u8 *next;

    next = AddTilePrim(ot, prim, x + 1, y + 2, 0xC2, 0x1C, 0, 0, 0);
    return AddTilePrim(ot, next, x, y, 0xC4, 0x20, 0xFF, 0xFF, 0xFF);
}
