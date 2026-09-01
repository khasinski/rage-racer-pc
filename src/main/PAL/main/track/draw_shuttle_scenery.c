#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"
#include "rage/render_world_game.h"

void DrawShuttleScenery(s32 instance) {
    GameShuttleScenery *state = &g_ShuttleScenery[instance];
    Matrix yaw;
    Matrix objectMatrix;
    Matrix worldMatrix;
    s32 modelId;

    if (!TrackCellVisible(state->position.x, state->position.z) &&
        g_CourseIndex != 2) {
        return;
    }

    BuildRotMatrixY(&yaw, state->angleY);
    BuildRotMatrixZ(&objectMatrix, state->angleZ);
    MulMatrix2(&yaw, &objectMatrix);
    worldMatrix = objectMatrix;
    MulMatrix2(&g_RenderState.matrix, &objectMatrix);

    modelId = SeriesCourseIndex() >= 2 ? 0x3C : 0x3F;
    if (modelId >= g_CourseModelCount) {
        modelId = 1;
    }

    SetGteObjectMatrix(&g_ObjectMatrixWork, AsPosition(&state->position),
                       &objectMatrix);
    g_RenderState.envMode4 = 0;
    GameRenderWorldSubmitDynamicCourseObject(
        0x110 + instance, modelId, state->position.x, state->position.y,
        state->position.z, worldMatrix.m, 0, 0);
    SubmitCourseModel(&g_RenderState, modelId);
}
