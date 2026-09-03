#include <assert.h>
#include <limits.h>

#include "game/car.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    STEP_CARS,
    STEP_TEXTURE,
    STEP_CAMERA,
    STEP_DRAW_CARS,
    STEP_ENVIRONMENT,
    STEP_SKY,
    STEP_TERRAIN,
    STEP_OBJECTS,
    STEP_SCENERY,
    STEP_COUNT,
};

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s32 g_AnimTimer;
s32 g_CameraCarIndex;
CameraViewMode g_CameraViewMode;
s32 g_IsEnvironmentMode4;
GameRenderState g_RenderState;

static s32 s_steps[STEP_COUNT];
static s32 s_stepCount;
static s32 s_textureSection;
static GameRenderObject *s_cameraObject;
static s32 s_sceneryTimer;
static s32 s_sceneryAnimate;

static void RecordStep(s32 step) {
    assert(s_stepCount < STEP_COUNT);
    s_steps[s_stepCount++] = step;
}

void UpdateAttractCars(void) { RecordStep(STEP_CARS); }
void RequestTrackTexturePage(s32 trackSection) {
    s_textureSection = trackSection;
    RecordStep(STEP_TEXTURE);
}
void UpdateCamera(CameraViewMode cameraMode, GameRenderObject *car) {
    assert(cameraMode == g_CameraViewMode);
    s_cameraObject = car;
    RecordStep(STEP_CAMERA);
}
void DrawCars(void) { RecordStep(STEP_DRAW_CARS); }
void UpdateEnvironment(void) { RecordStep(STEP_ENVIRONMENT); }
void DrawSkyBackground(void) { RecordStep(STEP_SKY); }
void DrawTerrainCellsWide(void) { RecordStep(STEP_TERRAIN); }
void DrawCourseObjects(void) { RecordStep(STEP_OBJECTS); }
void DrawPresentationCourseScenery(s32 timer, s32 animate) {
    s_sceneryTimer = timer;
    s_sceneryAnimate = animate;
    RecordStep(STEP_SCENERY);
}

static void Reset(void) {
    s_stepCount = 0;
    s_textureSection = -1;
    s_cameraObject = NULL;
    s_sceneryTimer = -1;
    s_sceneryAnimate = -1;
    g_AnimTimer = 123;
    g_CameraViewMode = CAMERA_VIEW_CHASE;
    g_IsEnvironmentMode4 = 7;
    g_RenderState.envMode4 = 0;
}

static void TestWorldUpdateOrder(void) {
    s32 step;

    Reset();
    g_CameraCarIndex = 2;
    g_Cars[2].trackSection = 45;

    UpdateAndDrawAttractWorld();

    assert(s_stepCount == STEP_COUNT);
    for (step = 0; step < STEP_COUNT; step++) {
        assert(s_steps[step] == step);
    }
    assert(s_textureSection == 45);
    assert(s_cameraObject == GetCarRenderObject(&g_Cars[2]));
    assert(g_RenderState.envMode4 == 7);
    assert(s_sceneryTimer == 123 && s_sceneryAnimate == 1);
}

static void TestInvalidCameraCarFallsBackToFirst(void) {
    Reset();
    g_CameraCarIndex = INT_MIN;
    g_Cars[0].trackSection = 11;

    UpdateAndDrawAttractWorld();

    assert(g_CameraCarIndex == 0);
    assert(s_textureSection == 11);
    assert(s_cameraObject == GetCarRenderObject(&g_Cars[0]));

    Reset();
    g_CameraCarIndex = INT_MAX;
    UpdateAndDrawAttractWorld();
    assert(g_CameraCarIndex == 0);
}

int main(void) {
    TestWorldUpdateOrder();
    TestInvalidCameraCarFallsBackToFirst();
    return 0;
}
