#include "common.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_BgmRandomLabelTimer;
s32 g_BgmSelectCursor;
s32 g_BgmSelectTrack;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

typedef struct SpriteRecord {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    s32 u;
    s32 v;
    s32 clut;
} SpriteRecord;

static GameFrameContext s_frame;
static u8 s_packets[128];
static SpriteRecord s_sprites[5];
static s32 s_spriteCount;
static s32 s_tileCount;
static s32 s_drawMode;

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                    s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    s_sprites[s_spriteCount++] =
        (SpriteRecord){x, y, width, height, u, v, clut};
    return prim + 1;
}

u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                       s32 height, s32 r, s32 g, s32 b) {
    (void)ot; (void)x; (void)y; (void)width; (void)height;
    (void)r; (void)g; (void)b;
    s_tileCount++;
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 texturePage) {
    (void)ot;
    s_drawMode = texturePage;
    return prim + 1;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_BgmRandomLabelTimer = 0;
    g_BgmSelectCursor = 1;
    g_BgmSelectTrack = 4;
    s_spriteCount = 0;
    s_tileCount = 0;
    s_drawMode = -1;
}

int main(void) {
    Reset();
    DrawBgmSelectBar();
    CHECK(s_spriteCount == 5 && s_tileCount == 1 && s_drawMode == 0xB);
    CHECK(s_sprites[0].x == 0x20 && s_sprites[0].u == 0);
    CHECK(s_sprites[1].x == 0x36 && s_sprites[1].u == 0x14);
    CHECK(s_sprites[2].x == 0x4C && s_sprites[2].u == 0x28);
    CHECK(s_sprites[0].clut == 0x3FEF);
    CHECK(s_sprites[1].clut == 0x3FEC);
    CHECK(s_sprites[2].clut == 0x3FEF);
    CHECK(s_sprites[3].v == 4 * 12 + 0x1C);

    Reset();
    g_BgmSelectCursor = 2;
    g_BgmRandomLabelTimer = 2;
    DrawBgmSelectBar();
    CHECK(g_BgmRandomLabelTimer == 2 && s_sprites[3].v == 0x10);
    CHECK(s_sprites[2].clut == 0x3FEC);

    UpdateBgmSelectBar();
    CHECK(g_BgmRandomLabelTimer == 1);
    UpdateBgmSelectBar();
    UpdateBgmSelectBar();
    CHECK(g_BgmRandomLabelTimer == 0);

    Reset();
    g_BgmRandomLabelTimer = INT_MIN;
    g_BgmSelectCursor = INT_MAX;
    g_BgmSelectTrack = INT_MAX;
    DrawBgmSelectBar();
    CHECK(s_sprites[0].clut == 0x3FEC);
    CHECK(s_sprites[3].v == 7 * 12 + 0x1C);
    UpdateBgmSelectBar();
    CHECK(g_BgmRandomLabelTimer == 0);

    puts("BGM selector bar preserves layout, highlight, and random label");
    return 0;
}
