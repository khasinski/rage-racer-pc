#include "game/render.h"
#include "game/track_internal.h"
#include "game/race.h"


void DrawFlybyScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    FlybySceneryState *state;

    state = &g_FlybyScenery;
    if (state->timer > 0) {
        BuildRotMatrixY(&mtx0, 0x800 - state->rotationY);
        BuildRotMatrixX(&mtx1, state->rotationX);
        MulMatrix2(&mtx0, &mtx1);
        MulMatrix2((&g_RenderState.matrix), &mtx1);
        BuildRotMatrixZ(&mtx0, state->rotationZ);
        MulMatrix2(&mtx1, &mtx0);
        SelectModelBank(2);
        SetGteObjectMatrix((&g_ObjectMatrixWork), AsPosition(&state->position), &mtx0);
        g_RenderState.envMode4 = 0;
        SubmitModel((&g_RenderState), g_ModelBankCount < 1);
    }
}
