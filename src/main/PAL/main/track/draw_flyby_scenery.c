#include "game/race.h"
#include "game/render.h"
#include "game/scenery_render_internal.h"
#include "game/track_internal.h"

void DrawFlybyScenery(void) {
    Matrix objectMatrix;
    FlybySceneryState *state = &g_FlybyScenery;

    if (state->timer <= 0) {
        return;
    }

    BuildSceneryObjectMatrix(&objectMatrix, state->rotationX,
                              state->rotationY, state->rotationZ);
    SelectModelBank(2);
    SetGteObjectMatrix(AsPosition(&state->position),
                       &objectMatrix);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0, g_ModelBankCount));
}
