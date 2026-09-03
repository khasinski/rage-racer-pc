#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scenery_render_internal.h"
#include "game/state.h"
#include "game/track_internal.h"

void DrawPathScenery(void) {
    Matrix objectMatrix;
    Matrix spinningPartMatrix;
    s32 spinAngle;

    BuildSceneryObjectMatrix(&objectMatrix,
                              g_PathSceneryTransform.rotation.vx,
                              g_PathSceneryTransform.rotation.vy,
                              g_PathSceneryTransform.rotation.vz);

    SelectModelBank(1);
    SetGteObjectMatrix(AsPositionWords(g_PathSceneryTransform.position.w),
                       &objectMatrix);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0x23, g_ModelBankCount));

    spinAngle = (s32)((u32)g_SceneTimer * 331u) & 0xFFF;
    BuildRotMatrixY(&spinningPartMatrix, spinAngle);

    MulMatrix2(&objectMatrix, &spinningPartMatrix);
    SetGteObjectMatrix(AsPositionWords(g_PathSceneryTransform.position.w),
                       &spinningPartMatrix);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0x24, g_ModelBankCount));
}
