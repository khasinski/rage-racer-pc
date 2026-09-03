#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

typedef struct SpinningSceneryRange {
    s32 first;
    s32 limit;
    s32 rateIndex;
} SpinningSceneryRange;

enum {
    SPINNER_ENTITY_BASE = 0x100,
    SPINNER_MODEL = 0x3E,
    SPINNER_RATE_REFRESH_MASK = 0x1FF,
    SINGLE_SPINNER_RATE_MASK = 0x1F,
    MULTIPLE_SPINNER_RATE_MASK = 0x3F,
};

static const SpinningSceneryRange s_singleSpinnerRange = {0, 1, 0};
static const SpinningSceneryRange s_multipleSpinnerRange = {1, 4, 1};

void DrawSpinningScenery(s32 timer, s32 animate) {
    Matrix yawMatrix;
    Matrix objectMatrix;
    Matrix worldMatrix;
    const SpinningSceneryRange *range;
    s32 spinner;
    const s32 modelId = ModelOrFallback(SPINNER_MODEL, g_CourseModelCount);

    range = SeriesCourseIndex() == 0
        ? &s_singleSpinnerRange
        : &s_multipleSpinnerRange;
    for (spinner = range->first; spinner < range->limit; spinner++) {
        const SpinningSceneryPlacement *placement =
            &g_SpinningSceneryPlacements[spinner];
        u32 angle = (u16)g_SpinningSceneryAngle[spinner];

        if (animate != 0) {
            angle += g_SpinningSceneryRate[range->rateIndex];
        }
        angle &= 0xFFF;
        g_SpinningSceneryAngle[spinner] = (s16)angle;

        BuildRotMatrixY(&yawMatrix, placement->yaw);
        BuildRotMatrixZ(&worldMatrix, (s32)angle);
        MulMatrix2(&yawMatrix, &worldMatrix);
        MulMatrix2(&g_RenderState.matrix, &yawMatrix);
        BuildRotMatrixZ(&objectMatrix, (s32)angle);
        MulMatrix2(&yawMatrix, &objectMatrix);
        SetGteObjectMatrix(&placement->position,
                           &objectMatrix);

        g_RenderState.envMode4 = 0;
        GameRenderWorldSubmitDynamicCourseObject(
            SPINNER_ENTITY_BASE + spinner, modelId, placement->position.x,
            placement->position.y, placement->position.z,
            worldMatrix.m, 1, 0);
        SubmitCourseModel2(&g_RenderState, modelId);
    }

    if ((timer & SPINNER_RATE_REFRESH_MASK) == 0 && animate != 0) {
        g_SpinningSceneryRate[0] = Random15() & SINGLE_SPINNER_RATE_MASK;
        g_SpinningSceneryRate[1] = Random15() & MULTIPLE_SPINNER_RATE_MASK;
    }
}
