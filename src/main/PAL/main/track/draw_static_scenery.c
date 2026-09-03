#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

enum {
    STANDARD_SCENERY_ENTITY_ID = 0,
    HIGH_CLASS_SCENERY_ENTITY_ID = 1,
    STANDARD_SCENERY_MODEL = 0x39,
    ENVIRONMENT_MODE4_SCENERY_MODEL = 0x3A,
    HIGH_CLASS_SCENERY_MODEL = 0x3F,
    SERIES_COURSE_Z_OFFSET = 0x5000,
    HIGH_CLASS_ENVIRONMENT_MODE = 0x10000,
};

static void SubmitStaticScenery(const LVec *sourcePosition, s32 yaw,
                                s32 worldObjectId, s32 modelId,
                                s32 environmentMode) {
    Matrix objectMatrix;
    Matrix worldMatrix;
    LVec position = *sourcePosition;

    BuildRotMatrixY(&objectMatrix, yaw);
    worldMatrix = objectMatrix;
    MulMatrix2(&g_RenderState.matrix, &objectMatrix);
    SetGteObjectMatrix(&position, &objectMatrix);

    if (g_IsEnvironmentMode4 != 0) {
        g_RenderState.envMode4 = environmentMode;
        GameRenderWorldSubmitDynamicCourseObject(
            worldObjectId, modelId, position.x, position.y, position.z,
            worldMatrix.m, 0, 0);
        SubmitCourseModel(&g_RenderState, modelId);
    } else {
        g_RenderState.envMode4 = 0;
        GameRenderWorldSubmitDynamicCourseObject(
            worldObjectId, modelId, position.x, position.y, position.z,
            worldMatrix.m, 1, 0);
        SubmitCourseModel2(&g_RenderState, modelId);
    }
}

void DrawStaticScenery(s32 shiftForSeriesCourse) {
    const SceneryPlacement *placement = &g_StaticSceneryState.standard;
    LVec position = placement->position;
    s32 modelId;

    if (shiftForSeriesCourse != 0) {
        position.z += SERIES_COURSE_Z_OFFSET;
    }
    if (!TrackCellVisible(position.x, position.z)) {
        return;
    }

    modelId = g_IsEnvironmentMode4 != 0 ? ENVIRONMENT_MODE4_SCENERY_MODEL
                                       : STANDARD_SCENERY_MODEL;
    SubmitStaticScenery(&position, placement->yaw, STANDARD_SCENERY_ENTITY_ID,
                        ModelOrFallback(modelId, g_CourseModelCount), 0);
}

void DrawHighClassScenery(void) {
    const SceneryPlacement *placement = &g_StaticSceneryState.highClass;

    SubmitStaticScenery(
        &placement->position, placement->yaw, HIGH_CLASS_SCENERY_ENTITY_ID,
        ModelOrFallback(HIGH_CLASS_SCENERY_MODEL, g_CourseModelCount),
        HIGH_CLASS_ENVIRONMENT_MODE);
}
