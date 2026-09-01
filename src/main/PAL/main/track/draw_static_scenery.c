#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

static s32 SceneryModelId(s32 modelCount, s32 modelLimit) {
    return modelCount < modelLimit ? 1 : modelLimit - 1;
}

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

void DrawStaticScenery(s32 shifted) {
    SceneryPlacement *placement = &g_StaticSceneryState.standard;
    LVec position = placement->position;
    s32 modelLimit;

    if (shifted != 0) {
        position.z += 0x5000;
    }
    if (!TrackCellVisible(position.x, position.z)) {
        return;
    }

    modelLimit = g_IsEnvironmentMode4 != 0 ? 0x3B : 0x3A;
    SubmitStaticScenery(&position, placement->yaw, 0,
                        SceneryModelId(g_CourseModelCount, modelLimit), 0);
}

void DrawHighClassScenery(void) {
    SceneryPlacement *placement = &g_StaticSceneryState.highClass;

    SubmitStaticScenery(
        &placement->position, placement->yaw, 1,
        SceneryModelId(g_CourseModelCount, 0x40), 0x10000);
}
