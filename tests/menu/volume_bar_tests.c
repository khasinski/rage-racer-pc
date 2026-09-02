#include "common.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

typedef struct SegmentRecord {
    s32 x;
    s32 y;
} SegmentRecord;

static GameFrameContext s_frame;
static u8 s_packets[64];
static SegmentRecord s_segments[32];
static s32 s_segmentCount;
static s32 s_capCount;
static s32 s_drawModeCount;
static s32 s_drawModes[2];
static s32 s_tileCount;
static s32 s_lastTileX;
static s32 s_lastTileY;

u8 *GameQueueSpriteTrans(void *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_capCount++;
    return prim + 1;
}

u8 *GameQueueSprite(void *ot, u8 *prim, s32 x, s32 y, s32 width,
                    s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_segments[s_segmentCount++] = (SegmentRecord){x, y};
    return prim + 1;
}

u8 *QueueDrawModePrim(void *ot, u8 *prim, s32 tpage) {
    (void)ot;
    s_drawModes[s_drawModeCount++] = tpage;
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height,
                s32 r, s32 g, s32 b) {
    (void)ot;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_tileCount++;
    s_lastTileX = x;
    s_lastTileY = y;
    return prim + 1;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    g_DrawBuffer = &s_frame;
    RENDER_PRIM_CURSOR_AS(u8) = s_packets;
    s_segmentCount = 0;
    s_capCount = 0;
    s_drawModeCount = 0;
    s_tileCount = 0;
}

int main(void) {
    Reset();
    DrawVolumeBar(-1, 100);
    CHECK(s_capCount == 2 && s_segmentCount == 0);
    CHECK(s_drawModeCount == 2 && s_drawModes[0] == 0x3A &&
          s_drawModes[1] == 0x39);
    CHECK(s_tileCount == 2 && s_lastTileX == 0x46 && s_lastTileY == 100);
    CHECK(RENDER_PRIM_CURSOR_AS(u8) == s_packets + 6);

    Reset();
    DrawVolumeBar(7, 0xD0);
    CHECK(s_segmentCount == 8);
    CHECK(s_segments[0].x == 0x62 && s_segments[0].y == 0xD4);
    CHECK(s_segments[7].x == 0x9A && s_segments[7].y == 0xD4);
    CHECK(RENDER_PRIM_CURSOR_AS(u8) == s_packets + 14);

    puts("volume bar preserves its caps, segments, draw modes and frame");
    return 0;
}
