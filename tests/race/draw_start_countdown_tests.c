#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

#undef SetSprt

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
RenderBufferAddress g_TileStripBuffers[2];
u8 g_TileStripStorage[2 * 512 * sizeof(TILE)];
s32 g_CountdownBoardOffset;
u32 g_CountdownGlyphTable[64];
u32 g_CountdownDigitPatterns[16];
CVec g_CountdownCellColors[4];
s32 g_FrameParity;
s32 g_RacePaused;

static s32 s_addPrimCalls;
static s32 s_addPrimsCalls;
static s32 s_drawModeCalls;
static s32 s_spriteCalls;
static GameOrderingTableEntry *s_lastOrderingTable;
static void *s_firstTile;
static void *s_lastTile;

void SetSprt(SPRT *sprite) {
    memset(sprite, 0, sizeof(*sprite));
    sprite->code = 0x64;
}

void SetTile(TILE *tile) {
    memset(tile, 0, sizeof(*tile));
    tile->code = 0x60;
}

void AddPrim(void *orderingTable, void *primitive) {
    s_lastOrderingTable = orderingTable;
    (void)primitive;
    s_addPrimCalls++;
}

void AddPrims(void *orderingTable, void *first, void *last) {
    s_lastOrderingTable = orderingTable;
    s_firstTile = first;
    s_lastTile = last;
    s_addPrimsCalls++;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *orderingTable, u8 *packet,
                      s32 tpage) {
    s_lastOrderingTable = orderingTable;
    (void)tpage;
    s_drawModeCalls++;
    return packet + sizeof(DR_MODE);
}

u8 *GameQueueSprite(GameOrderingTableEntry *orderingTable, u8 *packet,
                    s32 x, s32 y, s32 width, s32 height, s32 u, s32 v,
                    s32 clut) {
    SPRT *sprite = (SPRT *)packet;

    s_lastOrderingTable = orderingTable;
    SetSprt(sprite);
    sprite->x0 = x;
    sprite->y0 = y;
    sprite->w = width;
    sprite->h = height;
    sprite->u0 = u;
    sprite->v0 = v;
    sprite->clut = clut;
    s_spriteCalls++;
    return (u8 *)(sprite + 1);
}

u8 *GameQueueTexturedRect(GameOrderingTableEntry *orderingTable, u8 *packet,
                          s32 x, s32 y, s32 width, s32 height, s32 u, s32 v,
                          s32 uSpan, s32 vSpan, s32 clut, s32 tpage) {
    (void)orderingTable;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)uSpan;
    (void)vSpan;
    (void)clut;
    (void)tpage;
    return packet + sizeof(POLY_FT4);
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(u8 *packets) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(g_TileStripStorage, 0, sizeof(g_TileStripStorage));
    g_TileStripBuffers[0].bytes = g_TileStripStorage;
    g_TileStripBuffers[1].bytes =
        g_TileStripStorage + 512 * sizeof(TILE);
    g_RenderState.packetCursor = packets;
    s_addPrimCalls = 0;
    s_addPrimsCalls = 0;
    s_drawModeCalls = 0;
    s_spriteCalls = 0;
    s_lastOrderingTable = NULL;
    s_firstTile = NULL;
    s_lastTile = NULL;
}

int main(void) {
    u8 packets[512] = {0};
    TILE *tiles;

    g_CountdownCellColors[0] = (CVec){1, 2, 3, 0x60};
    g_CountdownCellColors[1] = (CVec){11, 12, 13, 0x60};
    g_CountdownCellColors[2] = (CVec){21, 22, 23, 0x60};
    g_CountdownCellColors[3] = (CVec){31, 32, 33, 0x60};
    g_CountdownGlyphTable[16] = 1;

    ResetCalls(packets);
    DrawStartCountdown(104);
    CHECK(g_RenderState.packetCursor == packets);
    CHECK(s_drawModeCalls == 0 && s_spriteCalls == 0 && s_addPrimCalls == 0);

    ResetCalls(packets);
    g_FrameParity = 1;
    g_RacePaused = 0;
    g_CountdownBoardOffset = 99;
    DrawStartCountdown(120);
    tiles = g_TileStripBuffers[1].tile;
    CHECK(g_CountdownBoardOffset == 0);
    CHECK(tiles[0].r0 == 11 && tiles[0].g0 == 12 && tiles[0].b0 == 13);
    CHECK(tiles[1].r0 == 1 && tiles[1].g0 == 2 && tiles[1].b0 == 3);
    CHECK(s_drawModeCalls == 2 && s_spriteCalls == 2);
    CHECK(s_addPrimCalls == 7 && s_addPrimsCalls == 1);
    CHECK(s_firstTile == tiles && s_lastTile == tiles + 511);
    CHECK(s_lastOrderingTable == GamePrimaryOrderingTable(1));
    CHECK((u8 *)g_RenderState.packetCursor > packets);

    ResetCalls(packets);
    g_FrameParity = 0;
    g_RacePaused = 1;
    g_CountdownBoardOffset = 0;
    DrawStartCountdown(120);
    CHECK(s_addPrimsCalls == 0);

    ResetCalls(packets);
    g_RacePaused = 0;
    g_CountdownBoardOffset = 0;
    DrawStartCountdown(240);
    CHECK(g_CountdownBoardOffset == -16);
    CHECK(s_addPrimsCalls == 0);

    puts("start countdown emits its board, lamps, and tile strip");
    return 0;
}
