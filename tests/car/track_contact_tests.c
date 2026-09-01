/* Replay-facing track reconstruction, swept over straights and both arcs. */

#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render_state.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

ObjectMatrixWork g_ObjectMatrixWork;
CarTrackWork g_CarTrackWork;
GameRenderState g_RenderState;

MATRIX *MulMatrix0(MATRIX *a, MATRIX *b, MATRIX *out) {
    (void)a;
    (void)b;
    return out;
}

void GameRenderWorldSetCamera(s32 x, s32 y, s32 z, s32 pitch, s32 yaw,
                              s32 roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

static GameTrackPoint s_points[8];
static GameTrackArcCenter s_arcs[2];
static u32 s_digest = 2166136261U;

static void Fold(s32 value) {
    int byte;
    for (byte = 0; byte < 4; byte++) {
        s_digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        s_digest *= 16777619U;
    }
}

static void BuildTrack(void) {
    int index;

    memset(s_points, 0, sizeof(s_points));
    memset(s_arcs, 0, sizeof(s_arcs));
    for (index = 0; index < 8; index++) {
        s_points[index].x = index * 0x1000;
        s_points[index].z = 0x800;
        s_points[index].y = (s16)(index * 8);
        s_points[index].angle = (s16)(index * 0x40);
        s_points[index].surfacePitch = (s16)(index * 4);
        s_points[index].crossSlope = (s16)(index - 4);
        s_points[index].leftHalfWidth = 0x400;
        s_points[index].rightHalfWidth = 0x500;
        s_points[index].segmentLength = 0x1000;
    }
    s_arcs[0].x = 0x4000;
    s_arcs[0].z = 0x4800;
    s_arcs[1].x = 0x5000;
    s_arcs[1].z = -0x3800;
    s_points[4].arcRef = 1;
    s_points[5].arcRef = (1 << 4) | 2;
    g_TrackPoints = s_points;
    g_TrackPointCount = 8;
    g_TrackArcCenters = s_arcs;
    g_TrackLength = 8 * 0x1000;
}

int main(void) {
    static const s32 alongValues[] = {0, 0x600, 0x1200};
    static const s32 lateralValues[] = {-0x600, -0x200, 0, 0x200, 0x600};
    static const s32 yaws[] = {0, 0x400, 0x800, 0xC00};
    static const u32 expected = 79163237U;
    GameCarRuntime car;
    int series, point, along, lateral, yaw;
    int calls = 0;

    BuildTrack();
    for (series = 0; series < 2; series++)
    for (point = 0; point < 8; point++)
    for (along = 0; along < 3; along++)
    for (lateral = 0; lateral < 5; lateral++)
    for (yaw = 0; yaw < 4; yaw++) {
        memset(&car, 0, sizeof(car));
        memset(&g_CarTrackWork, 0, sizeof(g_CarTrackWork));
        g_RaceSeries = series;
        car.trackPointIndex = point;
        car.x = s_points[point].x + alongValues[along];
        car.z = s_points[point].z + lateralValues[lateral];
        car.bodyYaw = yaws[yaw];
        car.progressA = point * 0x1000;
        car.trackProgress = car.progressA;

        ResetCarTrackState(&car);

        Fold(car.modelPitch);
        Fold(car.modelYaw);
        Fold(car.modelRoll);
        Fold(car.trackHeading.value);
        Fold(car.previousTrackProgress);
        Fold(car.trackProgress);
        Fold(car.trackSection);
        Fold(car.progressB);
        Fold(g_CarTrackWork.heading);
        Fold(g_CarTrackWork.leftHalfWidth);
        Fold(g_CarTrackWork.rightHalfWidth);
        Fold(g_CarTrackWork.crossSlope);
        Fold(g_CarTrackWork.surfacePitch);
        Fold(g_CarTrackWork.camberAngle);
        calls++;
    }

    if (s_digest != expected) {
        printf("FAIL: %d reset track states digest to %u, expected %u\n",
               calls, s_digest, expected);
        return 1;
    }
    printf("all %d reset track states preserved\n", calls);
    return 0;
}
