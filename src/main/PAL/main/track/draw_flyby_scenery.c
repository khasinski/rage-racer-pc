#include "common.h"
#include "game/render.h"
#include "game/track_internal.h"
#include "game/race.h"
#include "game/render_workspace.h"
#include "game/scratchpad_legacy.h"
#include "game/track.h"
#include "psyq/gte.h"


void DrawFlybyScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    FlybySceneryState *state;

    state = &g_FlybyScenery;
    if (state->timer > 0) {
        BuildRotMatrixY(&mtx0, 0x800 - state->rotationY);
        BuildRotMatrixX(&mtx1, state->rotationX);
        MulMatrix2(&mtx0, &mtx1);
        MulMatrix2(RENDER_VIEW_MATRIX_GTE, &mtx1);
        BuildRotMatrixZ(&mtx0, state->rotationZ);
        MulMatrix2(&mtx1, &mtx0);
        SelectModelBank(2);
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &state->position, &mtx0);
        RENDER_ENV_MODE4 = 0;
        SubmitModel(RENDER_WORKSPACE, g_ModelBankCount < 1);
    }
}

/* 0 while the route prop is not running; the seeder sets it to 1 and
 * UpdateRouteScenery increments it every frame, so it is both the enable
 * and the frame count since the seed. */

void SeedRouteScenery(void) {
    s32 series0;
    s32 series1;
    SceneryMotionData *data;
    SceneryMotionKeyframe *keyframe;
    s32 keyframeIndex;
    s32 value;

    g_RouteSceneryArmed = 1;
    g_RouteSceneryClock = 1;

    series0 = g_RaceSeries;
    data = g_RouteSceneryData;
    series1 = g_RaceSeries;
    SetRouteSceneryPosition(&(series0 + data->start)->position);

    keyframeIndex = (series1 + data->firstKeyframe)[0]
        [(g_RouteSceneryKeyIndex = 0, g_RouteSceneryFrame = 0, 0)];
    keyframe = &data->keyframes[keyframeIndex];

    g_RouteSceneryRotX = keyframe->rotationX;
    g_RouteSceneryRotY = RAW(keyframe->rotationY);
    value = RAW(keyframe->rotationZ);
    g_RouteSceneryKeyframe = keyframe;
    g_RouteSceneryRotZ = value;
}
