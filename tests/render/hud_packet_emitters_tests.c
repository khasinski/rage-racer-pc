#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;

static void *s_queueOt;
static u8 *s_queuePacket;
static s32 s_queueTpage;
static s32 s_tileCallCount;
static u8 *s_tilePacket;
static s32 s_tileArgs[7];

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    s_queueOt = ot;
    s_queuePacket = packet;
    s_queueTpage = tpage;
    return packet + 4;
}

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y, s32 width,
                       s32 height, s32 red, s32 green, s32 blue) {
    s_queueOt = ot;
    s_tilePacket = packet;
    s_tileArgs[0] = x;
    s_tileArgs[1] = y;
    s_tileArgs[2] = width;
    s_tileArgs[3] = height;
    s_tileArgs[4] = red;
    s_tileArgs[5] = green;
    s_tileArgs[6] = blue;
    s_tileCallCount++;
    return packet + 8;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetPackets(void *packet) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.packetCursor = packet;
    s_queueOt = NULL;
    s_queuePacket = NULL;
    s_queueTpage = -1;
    s_tileCallCount = 0;
    s_tilePacket = NULL;
    memset(s_tileArgs, 0, sizeof(s_tileArgs));
}

int main(void) {
    u8 packets[256];
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    SPRT_8 *digit;
    SPRT *warning;
    TILE *fade;

    memset(packets, 0, sizeof(packets));
    ResetPackets(packets);
    CHECK(DrawHudDigit(packets, 12, 34, 7, 0x456) ==
          packets + sizeof(SPRT_8));
    digit = (SPRT_8 *)packets;
    CHECK(digit->x0 == 12 && digit->y0 == 34);
    CHECK(digit->u0 == 56 && digit->v0 == 0x10 && digit->clut == 0x456);
    CHECK(digit->code == 0x75);

    memset(packets, 0, sizeof(packets));
    ResetPackets(packets);
    DrawFullscreenFadeTile(-20, 0x49);
    fade = (TILE *)packets;
    CHECK(fade->x0 == 0 && fade->y0 == 0);
    CHECK(fade->w == 320 && fade->h == 240);
    CHECK(fade->r0 == 0 && fade->g0 == 0 && fade->b0 == 0);
    CHECK(fade->code == 0x62);
    CHECK(s_queueOt == ot && s_queuePacket == (u8 *)(fade + 1));
    CHECK(s_queueTpage == 0x49);
    CHECK(g_RenderState.packetCursor == s_queuePacket + 4);

    memset(packets, 0, sizeof(packets));
    ResetPackets(packets);
    DrawFullscreenFadeTile(300, 9);
    fade = (TILE *)packets;
    CHECK(fade->r0 == 0xFF && fade->g0 == 0xFF && fade->b0 == 0xFF);

    memset(packets, 0, sizeof(packets));
    ResetPackets(packets);
    DrawWrongWayWarning();
    warning = (SPRT *)packets;
    CHECK(warning[0].x0 == 0x6C && warning[0].y0 == 0x78);
    CHECK(warning[0].u0 == 0xF0 && warning[0].v0 == 0x48);
    CHECK(warning[0].w == 0x10 && warning[0].h == 0x10);
    CHECK(warning[1].x0 == 0x7C && warning[1].v0 == 0x58);
    CHECK(warning[2].x0 == 0x8C && warning[2].u0 == 0xB8);
    CHECK(warning[2].v0 == 0x68 && warning[2].w == 0x48);
    CHECK(s_tileCallCount == 1 && s_tilePacket == (u8 *)(warning + 3));
    CHECK(s_tileArgs[0] == 0x64 && s_tileArgs[1] == 0x70);
    CHECK(s_tileArgs[2] == 0x78 && s_tileArgs[3] == 0x20);
    CHECK(s_tileArgs[4] == 8 && s_tileArgs[5] == 8 && s_tileArgs[6] == 8);
    CHECK(s_queueTpage == 9);
    CHECK(g_RenderState.packetCursor == s_queuePacket + 4);

    puts("HUD packet emitter tests passed");
    return 0;
}
