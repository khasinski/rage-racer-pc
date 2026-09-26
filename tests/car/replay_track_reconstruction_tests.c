/* Replay-facing track reconstruction, swept over straights and both arcs. */

#include "common.h"
#include "game/car.h"
#include "game/car_track_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static GameTrackPoint s_points[8];
static GameTrackArcCenter s_arcs[2];
static TrackRoute route;
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
    s_points[4].arcRef = TRACK_CURVE_PRIMARY;
    s_points[5].arcRef = (1 << 4) | TRACK_CURVE_MIRRORED;
    route.points = s_points;
    route.count = 8;
    route.arcs = s_arcs;
    route.length = 8 * 0x1000;
}

int main(void) {
    static const s32 alongValues[] = {0, 0x600, 0x1200};
    static const s32 lateralValues[] = {-0x600, -0x200, 0, 0x200, 0x600};
    static const s32 yaws[] = {0, 0x400, 0x800, 0xC00};
    static const u32 expected = 1581599701U;
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
        car.trackPointIndex = point;
        car.x = s_points[point].x + alongValues[along];
        car.z = s_points[point].z + lateralValues[lateral];
        car.bodyYaw = yaws[yaw];
        car.progressA = point * 0x1000;
        car.trackProgress = car.progressA;

        ReconstructCarTrackState(&car, &route, series);

        Fold(car.modelPitch);
        Fold(car.modelYaw);
        Fold(car.modelRoll);
        Fold(car.trackHeading);
        Fold(car.previousTrackProgress);
        Fold(car.trackProgress);
        Fold(car.trackSection);
        Fold(car.progressB);
        calls++;
    }

    if (s_digest != expected) {
        printf("FAIL: %d reset track states digest to %u, expected %u\n",
               calls, s_digest, expected);
        return 1;
    }

    memset(&car, 0, sizeof(car));
    car.modelYaw = 123;
    route.count = 0;
    ReconstructCarTrackState(&car, &route, 0);
    if (car.modelYaw != 123) {
        puts("FAIL: empty track reset changed the car");
        return 1;
    }

    BuildTrack();
    memset(&car, 0, sizeof(car));
    car.x = s_points[0].x;
    car.z = s_points[0].z;
    car.progressA = INT_MAX;
    ReconstructCarTrackState(&car, &route, 0);
    if (car.trackProgress != 0xFFF) {
        printf("FAIL: wrapped replay progress is %d, expected %d\n",
               car.trackProgress, 0xFFF);
        return 1;
    }
    /* Run two independently configured routes without swapping host globals. */
    GameTrackPoint otherPoints[2] = {
        {.x = 100, .z = 200, .angle = 0x400, .segmentLength = 1024,
         .leftHalfWidth = 128, .rightHalfWidth = 128},
        {.x = 1124, .z = 200, .angle = 0x400, .segmentLength = 1024,
         .leftHalfWidth = 128, .rightHalfWidth = 128},
    };
    const TrackRoute other = {
        .points = otherPoints, .count = 2, .length = 2048,
    };
    GameCarRuntime initial = {0};
    initial.x = s_points[4].x + 100;
    initial.z = s_points[4].z + 200;
    initial.trackPointIndex = 4;
    initial.progressA = 5000;
    initial.bodyYaw = 0x300;
    GameCarRuntime isolated = initial;
    ReconstructCarTrackState(&isolated, &route, 0);

    GameCarRuntime second = {0};
    second.x = 400;
    second.z = 200;
    second.progressA = 2100;
    ReconstructCarTrackState(&second, &other, 1);
    if (second.trackHeading != 0x400 || second.modelYaw != 0 ||
        second.trackProgress < 0 || second.trackProgress >= other.length) {
        puts("FAIL: independent route used the wrong geometry or length");
        return 1;
    }
    GameCarRuntime interleaved = initial;
    ReconstructCarTrackState(&interleaved, &route, 0);
    if (memcmp(&isolated, &interleaved, sizeof(isolated)) != 0) {
        puts("FAIL: interleaved routes changed reconstruction");
        return 1;
    }
    TrackRoute missingArcs = route;
    missingArcs.arcs = NULL;
    interleaved = initial;
    ReconstructCarTrackState(&interleaved, &missingArcs, 0);
    if (memcmp(&initial, &interleaved, sizeof(initial)) != 0) {
        puts("FAIL: missing arc data partially changed the car");
        return 1;
    }
    ReconstructCarTrackState(NULL, &route, 0);
    ReconstructCarTrackState(&interleaved, NULL, 0);
    printf("all %d reset track states preserved\n", calls);

    return 0;
}
