#include "game/prim.h"
#include "game/render.h"

#include <limits.h>
#include <stdio.h>

s32 g_FrameParity;

static void *s_setPacket;
static Rect s_rect;
static void *s_orderingTable;
static void *s_addedPacket;

#undef SetDrawArea
void SetDrawArea(DR_AREA *packet, RECT *rect) {
    s_setPacket = packet;
    s_rect = *rect;
}

void AddPrim(void *orderingTable, void *packet) {
    s_orderingTable = orderingTable;
    s_addedPacket = packet;
}

static int Check(s32 parity, s32 width, s32 height, s16 expectedY,
                 s16 expectedWidth, s16 expectedHeight) {
    DrawPacket packets[2];
    GameOrderingTableEntry orderingTable = {0};
    u8 *next;

    g_FrameParity = parity;
    next = QueueDrawAreaPrim(&orderingTable, packets, 12, 34, width, height);
    if (s_setPacket == packets && s_addedPacket == packets &&
        s_orderingTable == &orderingTable && next == (u8 *)&packets[1] &&
        s_rect.x == 12 && s_rect.y == expectedY &&
        s_rect.w == expectedWidth && s_rect.h == expectedHeight) {
        return 0;
    }
    fprintf(stderr, "draw-area packet mismatch for parity %d\n", parity);
    return 1;
}

int main(void) {
    if (Check(0, 320, 96, 34, 320, 96)) return 1;
    if (Check(1, 320, 96, 274, 320, 96)) return 1;
    return Check(INT_MAX, INT_MAX, INT_MIN, -206, -1, 0);
}
