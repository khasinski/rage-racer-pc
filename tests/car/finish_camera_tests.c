#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_state.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;

static s32 s_interpolatedPoint;
static s32 s_atanCall;
static s32 s_trackStateCalls;
static s32 s_drawCalls;
static CarTrackLimits s_trackLimits;

void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    (void)weight;
    s_interpolatedPoint = pointIndex;
    out[0] = pointIndex * 100;
    out[1] = 0;
    out[2] = pointIndex * 200;
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

void SetCameraRotMatrix(void) {}
void SelectModelBank(s32 bank) {
    if (bank != 0) puts("FAIL: finish camera selected wrong model bank");
}
void DrawPlayerCarModel(GameRenderObject *obj) {
    (void)obj;
    s_drawCalls++;
}

static int RunCase(s32 cameraPoint, s32 backwards, s32 expectedPoint) {
    PlayerCarRuntime target;

    memset(&target, 0, sizeof(target));
    memset(&g_CameraCar, 0, sizeof(g_CameraCar));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&s_trackLimits, 0x7F, sizeof(s_trackLimits));
    target.facingBackwards = backwards;
    target.x = 1000;
    target.y = 2000;
    target.z = 3000;
    g_TrackPointCount = 10;
    g_CameraCarTrackPoint = cameraPoint;
    g_CameraCar.x = 100;
    g_CameraCar.y = 200;
    g_CameraCar.z = 300;
    g_CameraCar.positionW = 400;
    g_CameraCar.segmentFraction = 2048;
    g_CameraCarZ = 300;
    g_CameraCarSpeed = 256;
    g_CameraCarHeading = 0;
    s_atanCall = 0;
    s_trackStateCalls = 0;
    s_drawCalls = 0;

    UpdateFinishCamera(&target);

    if (s_interpolatedPoint != expectedPoint ||
        g_CameraCarHeading != 0x300 ||
        g_CameraCarStepX != 256 || g_CameraCarStepZ != 512 ||
        g_CameraCar.x != 101 || g_CameraCarZ != 302 ||
        g_RenderState.viewX != 101 || g_RenderState.viewY != 136 ||
        g_RenderState.viewZ != 300 || g_RenderState.reserved14 != 400 ||
        g_RenderState.viewAngleX != 0x100 ||
        g_RenderState.viewAngleY != 0x200 ||
        g_RenderState.viewAngleZ != 0 ||
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
    if (RunCase(1, 0, 9) || RunCase(9, 1, 1)) return 1;
    puts("finish camera behavior preserved");
    return 0;
}
