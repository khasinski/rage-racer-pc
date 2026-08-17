#include "common.h"
#include "game/asset.h"
#include "game/race.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/vector.h"
#include "psyq/gte.h"

void DrawRouteScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    Matrix *mtx1Ptr;
    s32 frameValue;
    s32 drawId;

    BuildRotMatrixY(&mtx0, 0x800 - g_RouteSceneryRotY);
    mtx1Ptr = &mtx1;
    BuildRotMatrixX(mtx1Ptr, g_RouteSceneryRotX);
    MulMatrix2(&mtx0, mtx1Ptr);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, mtx1Ptr);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix2(mtx1Ptr, &mtx0);
    SelectModelBank(1);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &g_RouteSceneryX, &mtx0);
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
    s32 value;
    s32 v1;
    AssetAddress angleAddress;

    state = &g_ShuttleScenery[0];
    if ((RageSeriesCourseIndex()) == 2) {
        g_ShuttleScenery[1].pathIndex = 2;
        g_ShuttleScenery[1].position = g_ShuttlePath2Points.endpoint[0];

        index = g_ShuttleScenery[1].pathIndex;
        g_ShuttleScenery[1].angleX = g_ShuttlePathAngles[index].vx;
        g_ShuttleScenery[1].angleY = g_ShuttlePathAngles[index].vy;
        value = g_ShuttlePathAngles[index].vz;
        g_ShuttleScenery[1].startEndpoint = 0;
        g_ShuttleScenery[1].travelStep = 0;
        g_ShuttleScenery[1].angleZ = value;
        v1 = g_ShuttlePathDwellMax[index];
        state->pathIndex = 1;
        g_ShuttleScenery[1].dwellCounter = v1;
    } else {

    state->pathIndex = 0;
    }
    state->position = g_ShuttlePathPoints[state->pathIndex].endpoint[0];
    value = state->pathIndex;
    value <<= 3;
    angleAddress.pointer = g_ShuttlePathAngles;
    angleAddress.bytes += value;
    v1 = angleAddress.shortVector->vx;
    asm("" ::: "memory");
    value = state->pathIndex;
    value <<= 3;
    state->angleX = v1;
    angleAddress.pointer = g_ShuttlePathAngles;
    angleAddress.bytes += value;
    v1 = angleAddress.shortVector->vy;
    value = state->pathIndex;
    value <<= 3;
    state->angleY = v1;
    angleAddress.pointer = g_ShuttlePathAngles;
    angleAddress.bytes += value;
    v1 = angleAddress.shortVector->vz;
    value = state->pathIndex;
    state->startEndpoint = 0;
    state->travelStep = 0;
    state->angleZ = v1;
    value = g_ShuttlePathDwellMax[value];
    state->dwellCounter = value;
}
