#include "game/asset.h"
#include "game/race.h"
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
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx1);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix2(&mtx1, &mtx0);
    SelectModelBank(1);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, AsPositionWords(&g_RouteSceneryX), &mtx0);
    frameValue = g_ModelBankCount;
    SCRATCH_ENV_MODE4 = 0;
    drawId = 1;
    if (frameValue >= 0x26) {
        drawId = 0x25;
    }
    SubmitModel(SCRATCHPAD, drawId);
}

void InitShuttleScenery(void) {
    GameShuttleScenery *state;
    s32 index;

    state = &g_ShuttleScenery[0];
    if ((SeriesCourseIndex()) == 2) {
        g_ShuttleScenery[1].pathIndex = 2;
        g_ShuttleScenery[1].position = g_ShuttlePath2Points.endpoint[0];

        index = g_ShuttleScenery[1].pathIndex;
        g_ShuttleScenery[1].angleX = g_ShuttlePathAngles[index].vx;
        g_ShuttleScenery[1].angleY = g_ShuttlePathAngles[index].vy;
        g_ShuttleScenery[1].startEndpoint = 0;
        g_ShuttleScenery[1].travelStep = 0;
        g_ShuttleScenery[1].angleZ = g_ShuttlePathAngles[index].vz;
        state->pathIndex = 1;
        g_ShuttleScenery[1].dwellCounter = g_ShuttlePathDwellMax[index];
    } else {
        state->pathIndex = 0;
    }
    state->position = g_ShuttlePathPoints[state->pathIndex].endpoint[0];
    state->angleX = g_ShuttlePathAngles[state->pathIndex].vx;
    state->angleY = g_ShuttlePathAngles[state->pathIndex].vy;
    state->angleZ = g_ShuttlePathAngles[state->pathIndex].vz;
    state->startEndpoint = 0;
    state->travelStep = 0;
    state->dwellCounter = g_ShuttlePathDwellMax[state->pathIndex];
}
