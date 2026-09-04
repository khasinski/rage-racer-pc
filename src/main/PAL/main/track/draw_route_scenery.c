#include "game/asset.h"
#include "game/render.h"
#include "game/scenery_render_internal.h"
#include "game/track_internal.h"

void DrawRouteScenery(void) {
    Matrix objectMatrix;

    BuildSceneryObjectMatrix(&objectMatrix, g_RouteSceneryRotX,
                              g_RouteSceneryRotY, g_RouteSceneryRotZ);
    SelectModelBank(1);
    SetGteObjectMatrix(AsPosition(&g_RouteSceneryPosition), &objectMatrix);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0x25, g_ModelBankCount));
}
