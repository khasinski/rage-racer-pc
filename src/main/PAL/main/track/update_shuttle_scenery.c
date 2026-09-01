#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"
#include "rage/render_world_game.h"

void UpdateShuttleScenery(s32 instance) {
    GameShuttleScenery *entry;
    s32 phase;
    s32 side;
    s32 step;
    s16 denom;
    const Vec4 *basePoint;
    const Vec4 *altPoint;

    entry = &g_ShuttleScenery[instance];
    
    side = entry->startEndpoint;
    phase = entry->pathIndex;
    step = entry->travelStep;
    denom = g_ShuttlePathTravelMax[phase];
    basePoint = &g_ShuttlePathPoints[phase].endpoint[side];
    altPoint = &g_ShuttlePathPoints[phase].endpoint[1 - side];
    entry->position.x = ((denom - step) * basePoint->x +
                         step * altPoint->x) / denom;

    entry->position.y = ((denom - step) * basePoint->y +
                         step * altPoint->y) / denom;

    entry->position.z = ((denom - step) * basePoint->z +
                         step * altPoint->z) / denom;

    if (entry->travelStep >= denom) {
        entry->travelStep = 0;
        entry->dwellCounter = 0;
        entry->startEndpoint ^= 1;
        return;
    }

    if (entry->dwellCounter >= g_ShuttlePathDwellMax[phase]) {
        entry->travelStep++;
        entry->dwellCounter = g_ShuttlePathDwellMax[phase];
        return;
    }
    entry->dwellCounter++;
}


void DrawShuttleScenery(s32 instance) {
    s32 drawArg;
    Matrix mtx0;
    Matrix mtx1;
    Matrix renderWorldMtx;
    GameShuttleScenery *state;
    Matrix *mtx1Ptr;
    s32 drawValue;
    s32 wordIndex;
    s32 bitIndex;
    s32 bit;
    s32 firstValue;
    s32 value;
    u32 *visibility;
    u32 *wordPtr;
    VisibilityMaskAddress visibilityAddress;
    s32 visible;
    s32 frameValue;

    state = &g_ShuttleScenery[instance];
    firstValue = state->position.z;
    wordIndex = firstValue + 0x400;
    if (wordIndex < 0) {
        wordIndex = firstValue + 0xBFF;
    }
    wordIndex >>= 11;
    value = state->position.x;
    visibility = g_VisibleCellMask;
    bit = value + 0x400;
    visibilityAddress.pointer = visibility;
    visibilityAddress.value =
        (wordIndex << 2) + visibilityAddress.value;
    wordPtr = visibilityAddress.pointer;
    if (bit < 0) {
        bit = value + 0xBFF;
    }
    bitIndex = bit >> 11;
    visible = 1 << bitIndex;
    visible &= *wordPtr;

    if ((visible != 0) || (g_CourseIndex == 2)) {
        drawArg = 0x3F;
        BuildRotMatrixY(&mtx0, state->angleY);
        mtx1Ptr = &mtx1;
        BuildRotMatrixZ(mtx1Ptr, state->angleZ);
        MulMatrix2(&mtx0, mtx1Ptr);
        renderWorldMtx = *mtx1Ptr;
        MulMatrix2((&g_RenderState.matrix), mtx1Ptr);
        if ((SeriesCourseIndex()) >= 2) {
            drawArg = 0x3C;
        }
        SetGteObjectMatrix((&g_ObjectMatrixWork), AsPosition(&state->position), mtx1Ptr);
        frameValue = g_CourseModelCount;
        g_RenderState.envMode4 = 0;
        drawValue = 1;
        if (drawArg < frameValue) {
            drawValue = drawArg;
        }
        GameRenderWorldSubmitDynamicCourseObject(
            0x110 + instance, drawValue, state->position.x,
            state->position.y, state->position.z, renderWorldMtx.m, 0, 0);
        SubmitCourseModel((&g_RenderState), drawValue);
    }
}
