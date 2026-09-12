#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_state.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;

static s32 s_interpolatedPoint;
static s32 s_atanCall;
static s32 s_trackStateCalls;
static s32 s_drawCalls;
static CarTrackLimits s_trackLimits;
static GameTrackPoint s_trackPoint;

void InterpolateTrackPoint(s32 pointIndex, LVec *out, s32 weight) {
    (void)weight;
    s_interpolatedPoint = pointIndex;
    out->x = pointIndex * 100;
    out->y = 0;
    out->z = pointIndex * 200;
}

s32 Atan2(s32 x, s32 y) {
    static const s32 results[] = {0x100, 0x200, 0x300};
    (void)x;
    (void)y;
    return results[s_atanCall++];
}

s32 GetAngleDelta(s32 from, s32 to) { return to - from; }
s32 rsin(s32 angle) { (void)angle; return 256; }
s32 rcos(s32 angle) { (void)angle; return 512; }
long SquareRoot12(long value) { return value; }
void AccumulateLapProgress(GameCarRuntime *car) { (void)car; }

s32 UpdateCarTrackState(GameCarRuntime *car, s32 trackPointIndex,
                        const CarTrackLimits *limits) {
    (void)car;
    (void)trackPointIndex;
    s_trackStateCalls++;
    s_trackLimits = *limits;
    return 0;
}

void SetCameraRotMatrix(const GameCameraState *camera) { (void)camera;}
void SelectModelBank(s32 bank) {
    if (bank != 0) puts("FAIL: finish camera selected wrong model bank");
}
void DrawPlayerCarModel(GameCarRuntime *obj) {
    (void)obj;
    s_drawCalls++;
}

static int RunCase(s32 cameraPoint, s32 backwards, s32 expectedPoint) {
    PlayerCarRuntime target;
    Camera camera;
    FinishCamera finish;

    memset(&target, 0, sizeof(target));
    memset(&finish, 0, sizeof(finish));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&camera, 0, sizeof(camera));
    memset(&s_trackLimits, 0x7F, sizeof(s_trackLimits));
    target.facingBackwards = backwards;
    target.x = 1000;
    target.y = 2000;
    target.z = 3000;
    g_TrackPointCount = 10;
    g_TrackPoints = &s_trackPoint;
    finish.point = cameraPoint;
    finish.car.x = 100;
    finish.car.y = 200;
    finish.car.z = 300;
    finish.car.positionW = 400;
    finish.car.segmentFraction = 2048;
    finish.car.speed = 256;
    s_atanCall = 0;
    s_trackStateCalls = 0;
    s_drawCalls = 0;

    UpdateFinishCamera(&camera, &finish, &target);

    if (s_interpolatedPoint != expectedPoint ||
        finish.heading != 0x300 ||
        finish.car.x != 101 || finish.car.z != 302 ||
        camera.view.x != 101 || camera.view.y != 136 ||
        camera.view.z != 302 || camera.view.parameter != 400 ||
        camera.view.angleX != 0x100 ||
        camera.view.angleY != 0x200 ||
        camera.view.angleZ != 0 ||
        s_trackStateCalls != 1 || s_drawCalls != 1 ||
        memcmp(&s_trackLimits, &(CarTrackLimits){0},
               sizeof(s_trackLimits)) != 0) {
        printf("FAIL: point=%d backwards=%d target=%d\n",
               cameraPoint, backwards, s_interpolatedPoint);
        return 1;
    }
    return 0;
}

int main(void) {
    PlayerCarRuntime target = {0};
    Camera camera = {0};
    FinishCamera finish = {0};

    if (RunCase(1, 0, 9) || RunCase(9, 1, 1)) return 1;

    target.x = 1000;
    target.y = 2000;
    target.z = 3000;
    finish.car.speed = INT_MIN;
    s_atanCall = 0;
    UpdateFinishCamera(&camera, &finish, &target);
    if (finish.car.x != 0 || finish.car.z != 0) {
        puts("FAIL: finish camera velocity did not wrap as a PS1 word");
        return 1;
    }

    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    s_trackStateCalls = 0;
    s_drawCalls = 0;
    UpdateFinishCamera(&camera, &finish, &(PlayerCarRuntime){0});
    if (s_trackStateCalls != 0 || s_drawCalls != 0) {
        puts("FAIL: finish camera ran without a loaded track");
        return 1;
    }
    puts("finish camera behavior preserved");
    return 0;
}
