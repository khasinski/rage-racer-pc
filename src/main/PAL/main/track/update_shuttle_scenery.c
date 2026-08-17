#include "common.h"
#include "game/asset.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/track.h"
#include "psyq/gte.h"

void UpdateShuttleScenery(s32 instance) {
    GameShuttleScenery *entry;
    s32 phase;
    s32 side;
    s32 step;
    s16 *limitPtr;
    s16 *tailLimitPtr;
    s16 denom;
    const Vec4 *basePoint;
    const Vec4 *altPoint;

    entry = &g_ShuttleScenery[instance];
    asm("" : "=r"(entry) : "0"(entry));
    limitPtr = g_ShuttlePathTravelMax;
    side = entry->startEndpoint;
    phase = entry->pathIndex;
    step = entry->travelStep;
    limitPtr = &limitPtr[phase];
    denom = *limitPtr;
    basePoint = &g_ShuttlePathPoints[phase].endpoint[side];
    altPoint = &g_ShuttlePathPoints[phase].endpoint[1 - side];
    entry->position.x = ((denom - step) * basePoint->x +
                         step * altPoint->x) / denom;

    denom = *limitPtr;
    entry->position.y = ((denom - step) * basePoint->y +
                         step * altPoint->y) / denom;

    denom = *limitPtr;
    entry->position.z = ((denom - step) * basePoint->z +
                         step * altPoint->z) / denom;

    if (entry->travelStep >= *limitPtr) {
        entry->travelStep = 0;
        entry->dwellCounter = 0;
        entry->startEndpoint ^= 1;
        return;
    }

    tailLimitPtr = &g_ShuttlePathDwellMax[phase];
    if (entry->dwellCounter >= *tailLimitPtr) {
        entry->travelStep = entry->travelStep + 1;
        entry->dwellCounter = *tailLimitPtr;
        return;
    }
    entry->dwellCounter = entry->dwellCounter + 1;
}


void DrawShuttleScenery(s32 instance) {
    s32 drawArg;
    Matrix mtx0;
    Matrix mtx1;
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
        MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, mtx1Ptr);
        if ((RageSeriesCourseIndex()) >= 2) {
            drawArg = 0x3C;
        }
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &state->position, mtx1Ptr);
        frameValue = g_CourseModelCount;
        SCRATCH_ENV_MODE4 = 0;
        drawValue = 1;
        if (drawArg < frameValue) {
            drawValue = drawArg;
        }
        SubmitCourseModel(SCRATCHPAD, drawValue);
    }
}
