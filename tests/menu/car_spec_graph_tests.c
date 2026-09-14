#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <limits.h>

static CarModelAsset s_model;
CarModelAsset *g_CarModelAsset = &s_model;
GameRenderState g_RenderState;

static s32 s_quadCount;
static s32 s_spriteCount;
static s32 s_polyLineCount;
static s16 s_polyLineY[16];

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)textureU;
    (void)textureV;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_spriteCount++;
}

void DrawPolyLine3(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, s16 y1, s16 x2,
                   s16 y2, u8 r, u8 g, u8 b, u8 alpha) {
    (void)ot;
    (void)x0;
    s_polyLineY[s_polyLineCount] = y0;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
    s_polyLineCount++;
}

void DrawFlatQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                  u16 y2, u16 x3, u16 y3, u8 r, u8 g, u8 b, s32 semiTrans,
                  u32 flags) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)x3;
    (void)y3;
    (void)r;
    (void)g;
    (void)b;
    (void)semiTrans;
    (void)flags;
    s_quadCount++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void DrawGraph(CarSpecGraph *graph, s32 step, u32 tireGrade) {
    graph->step = step;
    DrawCarSpecGraph(graph, tireGrade);
}

int main(void) {
    CarSpecGraph graph = {0};
    static GameOrderingTableEntry orderingTable[4];

    RENDER_OT_BASE = orderingTable;
    graph.progress = 42;
    graph.bars[0] = 7;
    DrawGraph(&graph, 0, 0);
    CHECK(graph.progress == 0 && graph.bars[0] == 7);

    s_model.performanceRatings[0] = 3;
    s_model.performanceRatings[1] = 2;
    s_model.performanceRatings[2] = 1;
    graph.bars[0] = 0;
    graph.bars[1] = 3;
    graph.bars[2] = 1;
    graph.bars[3] = 0;
    RENDER_OT_BASE = NULL;
    DrawGraph(&graph, 1, 2);
    CHECK(graph.progress == 1);
    CHECK(graph.bars[0] == 1 && graph.bars[1] == 2);
    CHECK(graph.bars[2] == 1 && graph.bars[3] == 1);
    CHECK(s_spriteCount == 0 && s_quadCount == 0);
    RENDER_OT_BASE = orderingTable;

    DrawGraph(&graph, -5, 5);
    CHECK(graph.progress == 0);
    CHECK(graph.bars[0] == 2 && graph.bars[3] == 2);

    s_model.performanceRatings[0] = 10;
    s_model.performanceRatings[1] = 20;
    s_model.performanceRatings[2] = 30;
    graph.bars[0] = 10;
    graph.bars[1] = 20;
    graph.bars[2] = 30;
    graph.bars[3] = 50;
    graph.progress = 95;
    DrawGraph(&graph, 10, 2);
    CHECK(graph.progress == 96);
    CHECK(s_spriteCount == 8);
    CHECK(s_polyLineCount == 12);
    CHECK(s_polyLineY[0] == 0x13E && s_polyLineY[1] == 0x13F);
    CHECK(s_polyLineY[2] == 0x12E && s_polyLineY[3] == 0x12F);
    CHECK(s_polyLineY[10] == 0xEE && s_polyLineY[11] == 0xEF);
    CHECK(s_quadCount == 16);

    g_CarModelAsset = NULL;
    graph.progress = INT_MAX;
    graph.bars[0] = INT_MAX;
    graph.bars[1] = INT_MIN;
    graph.bars[2] = 2;
    graph.bars[3] = INT_MAX;
    DrawGraph(&graph, INT_MAX, 5);
    CHECK(graph.progress == 96);
    CHECK(graph.bars[0] == 95 && graph.bars[1] == 0);
    CHECK(graph.bars[2] == 1 && graph.bars[3] == 95);

    s_spriteCount = 0;
    s_quadCount = 0;
    RENDER_OT_BASE = NULL;
    DrawGraph(&graph, 1, UINT_MAX);
    CHECK(s_spriteCount == 0 && s_quadCount == 0);

    puts("car spec graph tests passed");
    return 0;
}
