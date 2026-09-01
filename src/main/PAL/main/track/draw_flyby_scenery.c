#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

void DrawFlybyScenery(void) {
    Matrix rotation;
    Matrix objectMatrix;
    FlybySceneryState *state = &g_FlybyScenery;

    if (state->timer <= 0) {
        return;
    }

    BuildRotMatrixY(&rotation, 0x800 - state->rotationY);
    BuildRotMatrixX(&objectMatrix, state->rotationX);
    MulMatrix2(&rotation, &objectMatrix);
    MulMatrix2(&g_RenderState.matrix, &objectMatrix);
    BuildRotMatrixZ(&rotation, state->rotationZ);
    MulMatrix2(&objectMatrix, &rotation);
    SelectModelBank(2);
    SetGteObjectMatrix(&g_ObjectMatrixWork, AsPosition(&state->position),
                       &rotation);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, g_ModelBankCount < 1);
}
