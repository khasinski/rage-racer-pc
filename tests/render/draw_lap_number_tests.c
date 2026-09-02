#include "common.h"
#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
PlayerCarRuntime g_PlayerCar;
GameFrameContext *g_DrawBuffer;

static GameFrameContext s_frame;
static u8 s_packets[sizeof(SPRT) * 8];
static u8 *s_drawModePacket;
static s32 s_drawModePage;
static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    (void)ot;
    s_drawModePacket = packet;
    s_drawModePage = tpage;
    return packet + sizeof(DrawPacket);
}

static void ResetPackets(s32 lap) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(s_packets, 0, sizeof(s_packets));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_PlayerCar.lap = lap;
    s_drawModePacket = NULL;
    s_drawModePage = -1;
}

static void CheckDigit(const SPRT *digit, s32 value, s32 x) {
    CHECK_EQ(digit->u0, value * 24, "digit texture u");
    CHECK_EQ(digit->v0, 0x48, "digit texture v");
    CHECK_EQ(digit->clut, 0x780B, "digit clut");
    CHECK_EQ(digit->x0, x, "digit x");
    CHECK_EQ(digit->y0, 0x10, "digit y");
    CHECK_EQ(digit->w, 0x18, "digit width");
    CHECK_EQ(digit->h, 0x20, "digit height");
}

static SPRT *PacketSprite(s32 index) {
    return (SPRT *)(s_packets + sizeof(SPRT) * index);
}

static void CheckZeroLap(void) {
    ResetPackets(0);
    DrawLapNumber();
    CheckDigit(PacketSprite(0), 0, 0x120);
    CHECK_EQ(s_drawModePacket == (u8 *)PacketSprite(1), 1,
             "zero lap packet count");
    CHECK_EQ(s_drawModePage, 9, "zero lap texture page");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_drawModePacket + sizeof(DrawPacket),
             1, "zero lap cursor includes draw mode");
}

static void CheckMultipleDigits(void) {
    ResetPackets(123);
    DrawLapNumber();
    CheckDigit(PacketSprite(0), 3, 0x120);
    CheckDigit(PacketSprite(1), 2, 0x108);
    CheckDigit(PacketSprite(2), 1, 0xF0);
    CHECK_EQ(s_drawModePacket == (u8 *)PacketSprite(3), 1,
             "multi-digit packet count");
    CHECK_EQ(s_drawModePage, 9, "multi-digit texture page");
    CHECK_EQ(g_RenderState.packetCursor ==
                 s_drawModePacket + sizeof(DrawPacket),
             1, "multi-digit cursor includes draw mode");
}

int main(void) {
    CheckZeroLap();
    CheckMultipleDigits();
    if (s_failures != 0) return 1;
    puts("lap number packets passed");
    return 0;
}
