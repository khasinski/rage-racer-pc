#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/rival.h"
#include "game/car_track_internal.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>


static LVec s_coords;
static s32 s_smoothAngle;
static s32 s_atanResult;
static s32 s_sampledIndex;
static s32 s_atanX;
static s32 s_atanZ;

void InterpolateRoutePoint(const TrackRoute *route, s32 index, LVec *out, s32 weight) {
    (void)route;
    (void)weight;
    s_sampledIndex = index;
    *out = s_coords;
}

s32 SmoothRouteAngle(const TrackRoute *route, s32 index, s32 weight) {
    (void)route;
    (void)weight;
    s_sampledIndex = index;
    return s_smoothAngle;
}

s32 Atan2(s32 x, s32 z) {
    s_atanX = x;
    s_atanZ = z;
    return s_atanResult;
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void ResetCar(GameCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    car->trackPointIndex = 0;
    car->normalizedLateralOffset = 1024;
    car->headingAngle = 500;
    car->bodyYaw = 600;
    car->targetYaw = 700;
    car->trackHeading = 100;
    s_coords.x = 1000;
    s_coords.y = 0;
    s_coords.z = 2000;
    s_smoothAngle = 0;
    s_atanResult = 300;
    s_sampledIndex = -1;
}

int main(void) {
    static GameTrackPoint points[5];
    GameCarRuntime car;
    TrackRoute route = {.points = points, .count = 5};
    int reverse = 0;
    s32 targetAngle = ANGLE_QUARTER_TURN - 300;
    s32 trackFacing;

    memset(points, 0, sizeof(points));
    route.count = 5;
    points[2].leftHalfWidth = 120;
    points[2].rightHalfWidth = 80;
    points[3] = points[2];

    ResetCar(&car);
    reverse = 0;
    car.aiLateralOffset = 100;
    car.x = 900;
    car.z = 1900;
    SteerRival(&car, &route, reverse);
    trackFacing = ANGLE_THREE_QUARTER_TURN - car.trackHeading;
    CHECK_EQ(s_sampledIndex, 3);
    CHECK_EQ(s_atanX, 100);
    CHECK_EQ(s_atanZ, 140);
    CHECK_EQ(car.steeringAngle,
             -GetAngleDelta(trackFacing, targetAngle) * 3);
    CHECK_EQ(car.headingAngle, targetAngle);
    CHECK_EQ(car.bodyYaw, targetAngle);
    CHECK_EQ(car.targetYaw, targetAngle);

    ResetCar(&car);
    reverse = 1;
    car.aiLateralOffset = -200;
    car.verticalMotionState = 2;
    car.x = 1000;
    car.z = 2000;
    SteerRival(&car, &route, reverse);
    CHECK_EQ(s_sampledIndex, 2);
    CHECK_EQ(s_atanX, 0);
    CHECK_EQ(s_atanZ, -60);
    CHECK_EQ(car.headingAngle, 500);
    CHECK_EQ(car.bodyYaw, 600);
    CHECK_EQ(car.targetYaw, 700);

    ResetCar(&car);
    route.count = 0;
    SteerRival(&car, &route, reverse);
    CHECK_EQ(s_sampledIndex, -1);
    CHECK_EQ(car.headingAngle, 500);

    route.count = 5;
    SteerRival(NULL, &route, reverse);
    CHECK_EQ(s_sampledIndex, -1);

    s_coords.x = INT_MAX;
    s_coords.z = INT_MIN;
    s_smoothAngle = 0;
    s_atanResult = INT_MIN;
    CHECK_EQ(CalculateRouteOffsetHeading(&route, 0, 0, INT_MIN, INT_MAX, 0),
             INT_MIN + ANGLE_QUARTER_TURN);
    CHECK_EQ(s_atanX, -1);
    CHECK_EQ(s_atanZ, 1);

    puts("route steering preserves direction, road limits, and airborne yaw");
    return 0;
}
