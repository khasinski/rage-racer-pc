#include "game/car_track_internal.h"
#include "game/render_state.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

const GameTrackArcCenter *g_TrackArcCenters;

s32 Atan2(s32 x, s32 y) {
    if (x < 0 && y == 0) {
        return 0x800;
    }
    if (x > 0 && y == 0) {
        return 0;
    }
    if (x == 0 && y < 0) {
        return 0xC00;
    }
    if (x == 0 && y > 0) {
        return 0x400;
    }
    return 0;
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    GameTrackArcCenter centers[2] = {
        {.x = 100, .z = 200},
        {.x = -50, .z = 75},
    };
    GameTrackPoint point = {.x = 100, .z = 300};
    GameTrackPoint nextPoint = {.x = 200, .z = 200};
    CarTrackWork work;

    memset(&work, 0x5A, sizeof(work));
    g_TrackArcCenters = centers;
    CarTrackMeasureArc(&work, 0, 0, 200, &point, &nextPoint);

    CHECK_EQ(work.arcCenterX, 100);
    CHECK_EQ(work.arcCenterZ, 200);
    CHECK_EQ(work.carToCenterX, -100);
    CHECK_EQ(work.carToCenterZ, 0);
    CHECK_EQ(work.pointToCenterX, 0);
    CHECK_EQ(work.pointToCenterZ, 100);
    CHECK_EQ(work.nextPointToCenterX, 100);
    CHECK_EQ(work.nextPointToCenterZ, 0);
    CHECK_EQ(work.carRadius.value, 100);
    CHECK_EQ(work.pointRadius.value, 100);
    CHECK_EQ(work.nextPointRadius.value, 100);
    CHECK_EQ(work.sweptAngle, 0x800);
    CHECK_EQ(work.pointAngle, 0x400);
    CHECK_EQ(work.nextPointAngle, 0);

    point.x = -50;
    point.z = 75;
    nextPoint = point;
    CarTrackMeasureArc(&work, 1, -50, 75, &point, &nextPoint);
    CHECK_EQ(work.carRadius.value, 0);
    CHECK_EQ(work.pointRadius.value, 0);
    CHECK_EQ(work.nextPointRadius.value, 0);

    centers[0].x = INT_MAX;
    centers[0].z = INT_MIN;
    point.x = INT_MIN;
    point.z = INT_MAX;
    nextPoint.x = INT_MAX;
    nextPoint.z = INT_MIN;
    CarTrackMeasureArc(&work, 0, INT_MIN, INT_MAX, &point, &nextPoint);
    CHECK_EQ(work.carToCenterX, 1);
    CHECK_EQ(work.carToCenterZ, -1);
    CHECK_EQ(work.pointToCenterX, 1);
    CHECK_EQ(work.pointToCenterZ, -1);
    CHECK_EQ(work.nextPointToCenterX, 0);
    CHECK_EQ(work.nextPointToCenterZ, 0);
    CHECK_EQ(work.carRadius.value, 1);
    CHECK_EQ(work.pointRadius.value, 1);
    CHECK_EQ(work.nextPointRadius.value, 0);

    puts("car arc measurement tests passed");
    return 0;
}
