#include "game/track.h"

#include <limits.h>
#include <stdio.h>

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    s32 expected_value = (s32)(expected);                                     \
    s32 actual_value = (s32)(actual);                                         \
    if (expected_value != actual_value) {                                     \
        fprintf(stderr, "%s:%d: expected %d, got %d\n",                     \
                __FILE__, __LINE__, expected_value, actual_value);             \
        failures++;                                                           \
    }                                                                         \
} while (0)

static void test_track_angle_interpolation(void) {
    TrackRoute route = {0};
    GameTrackPoint points[5] = {0};

    points[0].angle = 0xF00;
    points[1].angle = 0x100;
    points[2].angle = 0x500;
    points[3].angle = 0x500;
    points[4].angle = 0x700;
    route.points = points;
    route.count = 3;

    EXPECT_EQ(0, InterpolateRouteAngle(&route, 0, 0x200));
    EXPECT_EQ(0x300, InterpolateRouteAngle(&route, 1, 0x200));
    EXPECT_EQ(0x200, InterpolateRouteAngle(&route, 2, 0x200));
    EXPECT_EQ(0x100, InterpolateRouteAngle(&route, INT_MAX, 0x200));

    route.count = 5;
    points[2].angle = 0x300;
    EXPECT_EQ(0x280, SmoothRouteAngle(&route, 0, 0x200));
    EXPECT_EQ(0x4C0, SmoothRouteAngle(&route, 2, 0x200));

    route.points = NULL;
    EXPECT_EQ(0, InterpolateRouteAngle(&route, 0, 0x200));
    EXPECT_EQ(0, SmoothRouteAngle(&route, 0, 0x200));
    route.points = points;
    route.count = 0;
    EXPECT_EQ(0, InterpolateRouteAngle(&route, 0, 0x200));
}

static void test_track_point_interpolation(void) {
    TrackRoute route = {0};
    GameTrackPoint points[2] = {0};
    LVec out = {99, 99, 99};

    points[0].x = 10;
    points[0].y = 20;
    points[0].z = 30;
    points[1].x = -10;
    points[1].y = -20;
    points[1].z = -30;
    route.points = points;
    route.count = 2;

    InterpolateRoutePoint(&route, 0, &out, 0x200);
    EXPECT_EQ(0, out.x);
    EXPECT_EQ(0, out.y);
    EXPECT_EQ(0, out.z);

    InterpolateRoutePoint(&route, 1, &out, 0x100);
    EXPECT_EQ(-5, out.x);
    EXPECT_EQ(-5, out.y);
    EXPECT_EQ(-15, out.z);

    points[0].x = INT_MAX;
    points[0].y = SHRT_MIN;
    points[0].z = INT_MAX;
    points[1] = points[0];
    InterpolateRoutePoint(&route, INT_MAX, &out, INT_MAX);
    EXPECT_EQ(-1, out.x);
    EXPECT_EQ(-16384, out.y);
    EXPECT_EQ(-1, out.z);

    points[0].x = points[1].x = 10;
    points[0].y = points[1].y = 20;
    points[0].z = points[1].z = 30;
    InterpolateRoutePoint(&route, 0, &out, INT_MIN);
    EXPECT_EQ(10, out.x);
    EXPECT_EQ(10, out.y);
    EXPECT_EQ(30, out.z);

    route.points = NULL;
    InterpolateRoutePoint(&route, 0, &out, 0x200);
    EXPECT_EQ(0, out.x);
    EXPECT_EQ(0, out.y);
    EXPECT_EQ(0, out.z);
    InterpolateRoutePoint(&route, 0, NULL, 0x200);
}

int main(void) {
    test_track_angle_interpolation();
    test_track_point_interpolation();
    const GameTrackPoint firstPoints[2] = {
        {.x = 100, .z = 200, .angle = 0xF00},
        {.x = 300, .z = 400, .angle = 0x100},
    };
    const GameTrackPoint secondPoints[2] = {
        {.x = 1000, .z = 2000, .angle = 0x400},
        {.x = 3000, .z = 4000, .angle = 0x800},
    };
    const TrackRoute first = {.points = firstPoints, .count = 2};
    const TrackRoute second = {.points = secondPoints, .count = 2};
    LVec a;
    LVec b;
    InterpolateRoutePoint(&first, -2, &a, 512);
    InterpolateRoutePoint(&second, 2, &b, 512);
    EXPECT_EQ(200, a.x);
    EXPECT_EQ(300, a.z);
    EXPECT_EQ(2000, b.x);
    EXPECT_EQ(3000, b.z);
    EXPECT_EQ(0, InterpolateRouteAngle(&first, 0, 512));
    EXPECT_EQ(0x600, InterpolateRouteAngle(&second, 0, 512));
    EXPECT_EQ(0, SmoothRouteAngle(&first, 0, 512));
    EXPECT_EQ(0x600, SmoothRouteAngle(&second, 0, 512));
    InterpolateRoutePoint(&first, 0, &a, 512);
    EXPECT_EQ(200, a.x);
    EXPECT_EQ(0, SmoothRouteAngle(NULL, 0, 0));
    EXPECT_EQ(0, InterpolateRouteAngle(NULL, 0, 0));
    InterpolateRoutePoint(NULL, 0, &a, 0);
    EXPECT_EQ(0, a.x);
    return failures != 0;

}
