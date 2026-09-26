#include "game/angle.h"
#include "game/race.h"
#include "game/random.h"
#include "game/spinners.h"
#include <string.h>
#include "game/render.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

typedef struct SpinningSceneryRange {
    s32 first;
    s32 limit;
} SpinningSceneryRange;

enum {
    SPINNER_ENTITY_BASE = 0x100,
    SPINNER_MODEL = 0x3E,
};

static const SpinningSceneryRange s_singleSpinnerRange = {0, 1};
static const SpinningSceneryRange s_multipleSpinnerRange = {1, 4};

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
    Spinners state;
    memcpy(state.angles, g_SpinningSceneryAngle, sizeof(state.angles));
    memcpy(state.rates, g_SpinningSceneryRate, sizeof(state.rates));
    TickSpinners(&state, SeriesCourseIndex() != 0, (u32)timer, g_RandomSeed, animate);
    memcpy(g_SpinningSceneryAngle, state.angles, sizeof(state.angles));
    memcpy(g_SpinningSceneryRate, state.rates, sizeof(state.rates));
    for (spinner = range->first; spinner < range->limit; spinner++) {
        const SpinningSceneryPlacement *placement =
            &g_SpinningSceneryPlacements[spinner];
        u32 angle = (u16)g_SpinningSceneryAngle[spinner];

        BuildRotMatrixY(&yawMatrix, placement->yaw);
        BuildRotMatrixZ(&worldMatrix, (s32)angle);
        MulMatrix2(&yawMatrix, &worldMatrix);
        MulMatrix2(&g_RenderState.geometry.matrix, &yawMatrix);
        BuildRotMatrixZ(&objectMatrix, (s32)angle);
        MulMatrix2(&yawMatrix, &objectMatrix);
        SetGteObjectMatrix(&placement->position,
                           &objectMatrix);

        g_RenderState.geometry.envMode4 = 0;
        GameRenderWorldSubmitDynamicCourseObject(
            SPINNER_ENTITY_BASE + spinner, modelId, placement->position.x,
            placement->position.y, placement->position.z,
            worldMatrix.m, 1, 0);
        SubmitCourseModel2(&g_RenderState, modelId);
    }

}
