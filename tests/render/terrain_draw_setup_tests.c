#include "game/render.h"
#include "game/render_internal.h"
#include "game/terrain_internal.h"
#include "game/track.h"

#include <stdio.h>

GameRenderState g_RenderState;
Vec4 *g_VisibleCellList;

static s32 s_farDepth;
static s32 s_nearDepth;
static s32 s_rotMatrixCalls;
static s32 s_submitCount;
static void *s_submittedCells;

void BuildVisibleCells(s32 nearDepth, s32 farDepth) {
    s_nearDepth = nearDepth;
    s_farDepth = farDepth;
}
void SetRotMatrix(MATRIX *matrix) {
    if (matrix == &g_RenderState.matrix) s_rotMatrixCalls++;
}
void SubmitTerrainCells(void *state, void *cells, s32 count) {
    (void)state;
    s_submittedCells = cells;
    s_submitCount = count;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int CheckRange(s32 expectedNear, s32 expectedFar) {
    CHECK(s_nearDepth == expectedNear && s_farDepth == expectedFar);
    CHECK(s_rotMatrixCalls == 1);
    CHECK(s_submittedCells == g_VisibleCellList && s_submitCount == 64);
    s_rotMatrixCalls = 0;
    return 0;
}

int main(void) {
    Vec4 visibleCells[1];

    g_VisibleCellList = visibleCells;
    DrawTerrainCells();
    if (CheckRange(-0x3000, 0x14000)) return 1;

    DrawTerrainCellsWide();
    if (CheckRange(-0xA000, 0x14000)) return 1;

    DrawTerrainCellsInRange(-7, 1234);
    if (CheckRange(-7, 1234)) return 1;

    puts("terrain draw setup tests passed");
    return 0;
}
