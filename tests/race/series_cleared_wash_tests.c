#include "game/race.h"
#include "game/render_internal.h"

#include <stdio.h>

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

typedef struct TileCall {
    GameOrderingTableEntry *ot;
    s32 width, height, red, green, blue;
} TileCall;

static GameFrameContext s_frame;
static u8 s_packets[8];
static TileCall s_tile;
static void *s_drawModeOt;
static s32 s_tpage;

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                       s32 height, s32 red, s32 green, s32 blue) {
    (void)x;
    (void)y;
    s_tile = (TileCall){ot, width, height, red, green, blue};
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    s_drawModeOt = ot;
    s_tpage = tpage;
    return prim + 1;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void DrawAndCheck(s32 progress, s32 fade, s32 red, s32 green,
                         s32 blue) {
    g_RenderState.packetCursor = s_packets;
    DrawSeriesClearedWash(progress, fade);
    if (s_tile.red != red || s_tile.green != green || s_tile.blue != blue) {
        fprintf(stderr, "colors: got %d,%d,%d expected %d,%d,%d\n",
                s_tile.red, s_tile.green, s_tile.blue, red, green, blue);
    }
}

int main(void) {
    g_DrawBuffer = &s_frame;

    DrawAndCheck(80, 20, 20, 30, 40);
    CHECK(s_tile.red == 20 && s_tile.green == 30 && s_tile.blue == 40);
    CHECK(s_tile.ot == GamePrimaryOrderingTable(0));
    CHECK(s_tile.width == 0x140 && s_tile.height == 0xF0);
    CHECK(s_drawModeOt == s_tile.ot && s_tpage == 0x49);
    CHECK(g_RenderState.packetCursor == s_packets + 2);

    DrawAndCheck(2048, 300, 0xFF, 0xFF, 0xFF);
    CHECK(s_tile.red == 0xFF && s_tile.green == 0xFF &&
          s_tile.blue == 0xFF);

    DrawAndCheck(-80, -20, 0, 0, 0);
    CHECK(s_tile.red == 0 && s_tile.green == 0 && s_tile.blue == 0);

    puts("series-cleared wash tests passed");
    return 0;
}
