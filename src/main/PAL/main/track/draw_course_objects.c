#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

enum {
    TERRAIN_CELL_SIZE = 2048,
    COURSE_OBJECT_ENVIRONMENT_MATERIAL = 1 << 16,
};

void DrawCourseObjects(void) {
    Matrix objectMatrix;
    s32 i;

    if (g_CourseObjects == NULL || g_CourseObjectCount <= 0) return;

    for (i = 0; i < g_CourseObjectCount; i++) {
        CourseObject *object = &g_CourseObjects[i];
        s32 cellX;
        s32 cellZ;
        s32 flags;

        if (object->modelId == -1) continue;

        cellX = object->x / TERRAIN_CELL_SIZE;
        cellZ = object->z / TERRAIN_CELL_SIZE;
        if (!CellVisibilityMaskContains(g_VisibleCellMask, cellX, cellZ)) {
            continue;
        }

        BuildRotMatrixY(&objectMatrix, object->rotationY);
        MulMatrix2(&g_RenderState.matrix, &objectMatrix);
        SetGteObjectMatrix(AsPositionWords(&object->x), &objectMatrix);

        flags = object->flags;
        if (flags & COURSE_OBJECT_BLINK_ENVIRONMENT_4) {
            g_RenderState.envMode4 = (g_AnimTimer & 0x10) == 0
                ? COURSE_OBJECT_ENVIRONMENT_MATERIAL
                : 0;
        } else if (flags & COURSE_OBJECT_ENVIRONMENT_4) {
            g_RenderState.envMode4 = COURSE_OBJECT_ENVIRONMENT_MATERIAL;
        } else {
            g_RenderState.envMode4 = 0;
        }

        if (g_IsEnvironmentMode4
                ? (flags & COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4)
                : (flags & COURSE_OBJECT_ALTERNATE_NORMAL)) {
            SubmitCourseModel2(&g_RenderState, object->modelId);
        } else {
            SubmitCourseModel(&g_RenderState, object->modelId);
        }
    }
}
