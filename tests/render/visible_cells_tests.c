#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
u32 *g_VisibleCellMask;
Vec4 *g_VisibleCellList;
u16 *g_TerrainCellGrid;
CellVisibilityRow *g_CellVisibilityTable;
CourseObject *g_CourseObjects;
s32 g_CourseObjectCount;
s32 g_AnimTimer;
s32 g_IsEnvironmentMode4;

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)matrix;
    (void)angle;
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

#undef ApplyMatrix
void ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output) {
    (void)matrix;
    (void)input;
    (void)output;
}

void ApplyMatrixLV(void *matrix, const s32 *input, s32 *output) {
    (void)matrix;
    (void)input;
    (void)output;
}

void SetRotMatrix(MATRIX *matrix) { (void)matrix; }
void SetTransMatrix(MATRIX *matrix) { (void)matrix; }

void SubmitCourseModel(void *state, s32 model) {
    (void)state;
    (void)model;
}

void SubmitCourseModel2(void *state, s32 model) {
    (void)state;
    (void)model;
}

void GetVisibleCellScanOffset(s32 direction, s32 cellIndex, s32 rearView,
                              s32 offset[2]) {
    (void)direction;
    (void)cellIndex;
    (void)rearView;
    offset[0] = 0;
    offset[1] = 0;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    struct {
        u32 before;
        u32 values[TERRAIN_CELL_GRID_SIZE];
        u32 after;
    } mask;
    struct {
        Vec4 before;
        Vec4 values[VISIBLE_CELL_COUNT];
        Vec4 after;
    } list;
    s32 index;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&mask, 0xA5, sizeof(mask));
    memset(&list, 0x5A, sizeof(list));
    g_VisibleCellMask = mask.values;
    g_VisibleCellList = list.values;

    /* An out-of-grid camera returns immediately after clearing both outputs. */
    RENDER_VIEW_STATE->position.components.x.value = -2048;
    BuildVisibleCells(0, 1);

    CHECK(mask.before == 0xA5A5A5A5u && mask.after == 0xA5A5A5A5u);
    for (index = 0; index < TERRAIN_CELL_GRID_SIZE; index++) {
        CHECK(mask.values[index] == 0);
    }

    CHECK(list.before.x == 0x5A5A5A5A && list.after.w == 0x5A5A5A5A);
    for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
        CHECK(list.values[index].x == 0x5A5A5A5A);
        CHECK(list.values[index].w == -1);
    }

    puts("visible-cell output stays within its fixed buffers");
    return 0;
}
