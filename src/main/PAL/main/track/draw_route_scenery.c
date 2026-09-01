#include "game/asset.h"
#include "game/render.h"
#include "game/track_internal.h"

void DrawRouteScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    s32 frameValue;
    s32 drawId;

    BuildRotMatrixY(&mtx0, 0x800 - g_RouteSceneryRotY);
    BuildRotMatrixX(&mtx1, g_RouteSceneryRotX);
    MulMatrix2(&mtx0, &mtx1);
    MulMatrix2((&g_RenderState.matrix), &mtx1);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix2(&mtx1, &mtx0);
    SelectModelBank(1);
    SetGteObjectMatrix((&g_ObjectMatrixWork), AsPositionWords(&g_RouteSceneryX), &mtx0);
    frameValue = g_ModelBankCount;
    g_RenderState.envMode4 = 0;
    drawId = 1;
    if (frameValue >= 0x26) {
        drawId = 0x25;
    }
    SubmitModel((&g_RenderState), drawId);
}
