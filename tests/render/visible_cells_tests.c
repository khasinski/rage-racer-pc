#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
u32 *g_VisibleCellMask;
Vec4 *g_VisibleCellList;
u16 *g_TerrainCellGrid;
CellVisibilityRow *g_CellVisibilityTable;
CourseObject *g_CourseObjects;
s32 g_CourseObjectCount;
s32 g_AnimTimer;
s32 g_IsEnvironmentMode4;

typedef struct CourseObjectSubmission {
    s32 modelId;
    s32 alternate;
    s32 environmentMode4;
} CourseObjectSubmission;

static CourseObjectSubmission s_submissions[16];
static s32 s_submissionCount;
static s32 s_objectMatrixCount;

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)matrix;
    (void)angle;
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

void ApplyMatrixLV(const Matrix *matrix, const s32 *input, s32 *output) {
    (void)matrix;
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
}

void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    (void)position;
    (void)rotation;
    s_objectMatrixCount++;
}

void SubmitCourseModel(void *state, s32 model) {
    GameRenderState *renderState = state;
    CourseObjectSubmission *submission = &s_submissions[s_submissionCount++];

    submission->modelId = model;
    submission->alternate = 0;
    submission->environmentMode4 = renderState->envMode4;
}

void SubmitCourseModel2(void *state, s32 model) {
    GameRenderState *renderState = state;
    CourseObjectSubmission *submission = &s_submissions[s_submissionCount++];

    submission->modelId = model;
    submission->alternate = 1;
    submission->environmentMode4 = renderState->envMode4;
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

static int TestVisibleCellOutputBounds(void) {
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

    /* Missing terrain data produces the same empty frame for an in-grid
     * camera instead of dereferencing an uninstalled asset. */
    memset(mask.values, 0xA5, sizeof(mask.values));
    memset(list.values, 0x5A, sizeof(list.values));
    RENDER_VIEW_STATE->position.components.x.value = 0;
    RENDER_VIEW_STATE->position.components.z.value = 0;
    g_TerrainCellGrid = NULL;
    g_CellVisibilityTable = NULL;
    BuildVisibleCells(0, 1);
    for (index = 0; index < TERRAIN_CELL_GRID_SIZE; index++) {
        CHECK(mask.values[index] == 0);
    }
    for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
        CHECK(list.values[index].w == -1);
    }

    return 0;
}

static int TestMissingVisibleCellOutput(void) {
    g_VisibleCellMask = NULL;
    g_VisibleCellList = NULL;
    BuildVisibleCells(0, 1);
    return 0;
}

static int TestCameraHeightWrapsLikeThePs1(void) {
    u32 visibilityMask[TERRAIN_CELL_GRID_SIZE] = {0};
    Vec4 visibleCells[VISIBLE_CELL_COUNT];
    u16 terrainGrid[TERRAIN_CELL_GRID_SIZE * TERRAIN_CELL_GRID_SIZE] = {0};
    CellVisibilityRow cellVisibility[TERRAIN_CELL_GRID_SIZE] = {{0}};

    memset(visibleCells, 0, sizeof(visibleCells));
    terrainGrid[(TERRAIN_CELL_GRID_SIZE - 1) * TERRAIN_CELL_GRID_SIZE] = 5;
    cellVisibility[0][0] = 1;
    g_VisibleCellMask = visibilityMask;
    g_VisibleCellList = visibleCells;
    g_TerrainCellGrid = terrainGrid;
    g_CellVisibilityTable = cellVisibility;
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    RENDER_VIEW_STATE->position.components.y.value = INT_MAX;

    BuildVisibleCells(INT_MIN, INT_MAX);

    CHECK(visibleCells[0].y == 4);
    CHECK(visibleCells[0].w == 5);
    return 0;
}

static int TestCourseObjectFlags(void) {
    CourseObject objects[] = {
        {.modelId = -1},
        {.modelId = 10, .x = 2048},
        {.modelId = 11},
        {.modelId = 12, .flags = COURSE_OBJECT_ALTERNATE_NORMAL},
        {.modelId = 13,
         .flags = COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4},
        {.modelId = 14, .flags = COURSE_OBJECT_ENVIRONMENT_4},
        {.modelId = 15, .flags = COURSE_OBJECT_BLINK_ENVIRONMENT_4},
    };
    u32 visibility[TERRAIN_CELL_GRID_SIZE] = {1};

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(s_submissions, 0, sizeof(s_submissions));
    s_objectMatrixCount = 0;
    g_CourseObjects = objects;
    g_CourseObjectCount = sizeof(objects) / sizeof(objects[0]);
    g_VisibleCellMask = visibility;
    g_IsEnvironmentMode4 = 0;
    g_AnimTimer = 0;

    DrawCourseObjects();

    CHECK(s_submissionCount == 5 && s_objectMatrixCount == 5);
    CHECK(s_submissions[0].modelId == 11 && !s_submissions[0].alternate &&
          s_submissions[0].environmentMode4 == 0);
    CHECK(s_submissions[1].modelId == 12 && s_submissions[1].alternate);
    CHECK(s_submissions[2].modelId == 13 && !s_submissions[2].alternate);
    CHECK(s_submissions[3].modelId == 14 &&
          s_submissions[3].environmentMode4 == 0x10000);
    CHECK(s_submissions[4].modelId == 15 &&
          s_submissions[4].environmentMode4 == 0x10000);

    s_submissionCount = 0;
    s_objectMatrixCount = 0;
    g_IsEnvironmentMode4 = 1;
    g_AnimTimer = 0x10;
    DrawCourseObjects();

    CHECK(s_submissionCount == 5 && s_objectMatrixCount == 5);
    CHECK(!s_submissions[1].alternate);
    CHECK(s_submissions[2].alternate);
    CHECK(s_submissions[4].environmentMode4 == 0);
    return 0;
}

static int TestMissingCourseObjects(void) {
    u32 visibility[TERRAIN_CELL_GRID_SIZE] = {1};

    s_submissionCount = 0;
    s_objectMatrixCount = 0;
    g_VisibleCellMask = visibility;
    g_CourseObjects = NULL;
    g_CourseObjectCount = 1;
    DrawCourseObjects();
    CHECK(s_submissionCount == 0 && s_objectMatrixCount == 0);

    g_CourseObjectCount = 0;
    DrawCourseObjects();
    CHECK(s_submissionCount == 0 && s_objectMatrixCount == 0);
    return 0;
}

int main(void) {
    if (TestVisibleCellOutputBounds() != 0 ||
        TestMissingVisibleCellOutput() != 0 ||
        TestCameraHeightWrapsLikeThePs1() != 0 ||
        TestCourseObjectFlags() != 0 || TestMissingCourseObjects() != 0) {
        return 1;
    }
    puts("visible-cell bounds and course-object flags behave as expected");
    return 0;
}
