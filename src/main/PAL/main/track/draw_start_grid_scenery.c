#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

void DrawStartGridScenery(s32 timer) {
    Matrix objectMatrix;
    Matrix worldMatrix;
    Vec4 position;
    s32 animationFrame = 0;
    s32 movementStep = 0;
    s32 modelId;
    s32 series;

    if (g_RacePhase >= 2 || timer < 0x51) {
        return;
    }

    series = ReadStableRaceSeries();
    BuildRotMatrixY(&objectMatrix, g_StartGridSceneryAngle[series]);
    worldMatrix = objectMatrix;
    MulMatrix2(&g_RenderState.matrix, &objectMatrix);

    position = g_StartGridSceneryPos[series];
    if (timer > 90) {
        animationFrame = (timer - 90) / 3;
        movementStep = animationFrame / 15;
        position.x += g_StartGridSceneryStep[series].x * movementStep;
        position.z += g_StartGridSceneryStep[series].y * movementStep;
        animationFrame -= movementStep * 15;
    }
    if (SeriesCourseIndex() == 3) {
        position.z += 0x5000;
    }

    modelId = animationFrame + 0x28;
    if (modelId >= g_CourseModelCount) {
        modelId = 1;
    }
    SetGteObjectMatrix(&g_ObjectMatrixWork, AsPosition(&position),
                       &objectMatrix);
    g_RenderState.envMode4 = 0;
    GameRenderWorldSubmitDynamicCourseObject(
        2, modelId, position.x, position.y, position.z, worldMatrix.m, 0, 0);
    SubmitCourseModel(&g_RenderState, modelId);
}
