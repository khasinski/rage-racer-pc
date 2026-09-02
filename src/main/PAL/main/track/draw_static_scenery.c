#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

static void SubmitStaticScenery(const LVec *sourcePosition, s32 yaw,
                                s32 worldObjectId, s32 modelId,
                                s32 environmentMode) {
    Matrix objectMatrix;
    Matrix worldMatrix;
    LVec position = *sourcePosition;

    BuildRotMatrixY(&objectMatrix, yaw);
    worldMatrix = objectMatrix;
    MulMatrix2(&g_RenderState.matrix, &objectMatrix);
    SetGteObjectMatrix(&g_ObjectMatrixWork, &position, &objectMatrix);

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
        position.z += 0x5000;
    }
    if (!TrackCellVisible(position.x, position.z)) {
        return;
    }

    modelId = g_IsEnvironmentMode4 != 0 ? 0x3A : 0x39;
    SubmitStaticScenery(&position, placement->yaw, 0,
                        ModelOrFallback(modelId, g_CourseModelCount), 0);
}

void DrawHighClassScenery(void) {
    const SceneryPlacement *placement = &g_StaticSceneryState.highClass;

    SubmitStaticScenery(
        &placement->position, placement->yaw, 1,
        ModelOrFallback(0x3F, g_CourseModelCount), 0x10000);
}
