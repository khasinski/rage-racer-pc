#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

GameRenderState g_RenderState;
RaceIntroCameraScript *g_RaceIntroCameraScript;
RaceIntroCameraKey *g_RaceIntroCameraCursor;
SVec g_RaceIntroCameraDelta;
s32 g_RaceIntroCameraTimer;
s32 g_RaceSeries;

static int s_atanCalls;
static int s_matrixCalls;
static int s_drawCalls;
static int s_updateCameraCalls;
static int s_fadeCalls;
static s32 s_fadeColor;
static s32 s_selectedBank;

s32 Atan2(s32 x, s32 y) {
    (void)x;
    (void)y;
    return ++s_atanCalls * 100;
}

void SetCameraRotMatrix(void) { s_matrixCalls++; }

void SelectModelBank(s32 index) { s_selectedBank = index; }

void DrawPlayerCarModel(GameRenderObject *car) {
    (void)car;
    s_drawCalls++;
}

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)tpage;
    s_fadeCalls++;
    s_fadeColor = color;
}

void UpdateCamera(CameraViewMode mode, GameRenderObject *car) {
    (void)car;
    if (mode == CAMERA_VIEW_CAR) {
        s_updateCameraCalls++;
    }
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %lld, expected %lld\n", __LINE__,       \
                #actual, (long long)(intptr_t)(actual),                         \
                (long long)(intptr_t)(expected));                               \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    static struct {
        s16 firstKeyIndex[2];
        RaceIntroCameraKey keys[3];
    } script;
    PlayerCarRuntime car;
    s32 interpolation;

    memset(&script, 0, sizeof(script));
    memset(&car, 0, sizeof(car));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RaceIntroCameraScript = (RaceIntroCameraScript *)(void *)&script;
    script.firstKeyIndex[0] = 0;
    script.firstKeyIndex[1] = 1;
    script.keys[0].x.half.value = 100;
    script.keys[0].y.half.value = 200;
    script.keys[0].z.half.value = 300;
    script.keys[0].duration = 4;
    script.keys[0].startFrame = 2;
    script.keys[1].x.half.value = 500;
    script.keys[1].y.half.value = 600;
    script.keys[1].z.half.value = 700;
    script.keys[1].mode = 1;
    script.keys[1].duration = 3;
    car.x = 1000;
    car.y = 1100;
    car.z = 1200;

    RunRaceIntroCamera(&car, 0);
    interpolation = rcos(3 << 8);
    CHECK_EQ(g_RaceIntroCameraCursor, &script.keys[0]);
    CHECK_EQ(g_RaceIntroCameraTimer, 3);
    CHECK_EQ(g_RenderState.viewX, 100 + 400 * interpolation / 4096);
    CHECK_EQ(g_RenderState.viewY, 200 + 400 * interpolation / 4096);
    CHECK_EQ(g_RenderState.viewZ, 300 + 400 * interpolation / 4096);
    CHECK_EQ(g_RenderState.viewAngleY, ANGLE_QUARTER_TURN - 100);
    CHECK_EQ(g_RenderState.viewAngleX, ANGLE_QUARTER_TURN - 200);
    CHECK_EQ(s_drawCalls, 1);
    CHECK_EQ(s_selectedBank, 0);

    RunRaceIntroCamera(&car, 2);
    CHECK_EQ(g_RaceIntroCameraCursor, &script.keys[1]);
    CHECK_EQ(g_RaceIntroCameraTimer, 2);
    CHECK_EQ(g_RaceIntroCameraDelta.vx, 500);
    CHECK_EQ(g_RaceIntroCameraDelta.vy, 472);
    CHECK_EQ(g_RaceIntroCameraDelta.vz, 500);
    CHECK_EQ(g_RenderState.viewX, car.x);
    CHECK_EQ(g_RenderState.viewY, car.y - 28);
    CHECK_EQ(g_RenderState.viewZ, car.z);
    CHECK_EQ(s_fadeCalls, 1);
    CHECK_EQ(s_fadeColor, 52);
    CHECK_EQ(s_matrixCalls, 2);

    RunRaceIntroCamera(&car, 90);
    CHECK_EQ(s_updateCameraCalls, 1);
    CHECK_EQ(g_RaceIntroCameraTimer, 2);

    g_RaceIntroCameraScript = NULL;
    RunRaceIntroCamera(&car, 0);
    CHECK_EQ(s_updateCameraCalls, 2);
    g_RaceIntroCameraScript = (RaceIntroCameraScript *)(void *)&script;

    script.keys[0].duration = 0;
    script.keys[0].mode = 0;
    RunRaceIntroCamera(&car, 0);
    CHECK_EQ(g_RaceIntroCameraTimer, 0);
    CHECK_EQ(g_RenderState.viewX, script.keys[1].x.word);

    script.keys[0].x.word = INT_MAX;
    script.keys[1].x.half.value = 0xFFFF;
    script.keys[0].x.half.value = 0;
    script.keys[0].duration = 1;
    RunRaceIntroCamera(&car, 0);
    CHECK_EQ(g_RaceIntroCameraDelta.vx, -1);

    puts("race intro camera preserves script, fade, and handoff modes");
    return 0;
}
