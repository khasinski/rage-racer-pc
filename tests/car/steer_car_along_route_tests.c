#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_RaceSeries;

static s32 s_coords[3];
static s32 s_smoothAngle;
static s32 s_atanResult;
static s32 s_sampledIndex;
static s32 s_atanX;
static s32 s_atanZ;

void InterpolateTrackPoint(s32 index, s32 *out, s32 weight) {
    (void)weight;
    s_sampledIndex = index;
    memcpy(out, s_coords, sizeof(s_coords));
}

s32 SmoothTrackAngle(s32 index, s32 weight) {
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
    car->trackHeading.value = 100;
    s_coords[0] = 1000;
    s_coords[1] = 0;
    s_coords[2] = 2000;
    s_smoothAngle = 0;
    s_atanResult = 300;
    s_sampledIndex = -1;
}

int main(void) {
    static GameTrackPoint points[5];
    GameCarRuntime car;
    s32 targetAngle = ANGLE_QUARTER_TURN - 300;
    s32 trackFacing;

    memset(points, 0, sizeof(points));
    g_TrackPoints = points;
    g_TrackPointCount = 5;
    points[2].leftHalfWidth = 120;
    points[2].rightHalfWidth = 80;
    points[3] = points[2];

    ResetCar(&car);
    g_RaceSeries = 0;
    car.aiLateralOffset = 100;
    car.x = 900;
    car.z = 1900;
    SteerCarAlongRoute(&car);
    trackFacing = ANGLE_THREE_QUARTER_TURN - car.trackHeading.value;
    CHECK_EQ(s_sampledIndex, 3);
    CHECK_EQ(s_atanX, 100);
    CHECK_EQ(s_atanZ, 140);
    CHECK_EQ(car.steeringAngle,
             -GetAngleDelta(trackFacing, targetAngle) * 3);
    CHECK_EQ(car.headingAngle, targetAngle);
    CHECK_EQ(car.bodyYaw, targetAngle);
    CHECK_EQ(car.targetYaw, targetAngle);

    ResetCar(&car);
    g_RaceSeries = 1;
    car.aiLateralOffset = -200;
    car.verticalMotionState = 2;
    car.x = 1000;
    car.z = 2000;
    SteerCarAlongRoute(&car);
    CHECK_EQ(s_sampledIndex, 2);
    CHECK_EQ(s_atanX, 0);
    CHECK_EQ(s_atanZ, -60);
    CHECK_EQ(car.headingAngle, 500);
    CHECK_EQ(car.bodyYaw, 600);
    CHECK_EQ(car.targetYaw, 700);

    ResetCar(&car);
    g_TrackPointCount = 0;
    SteerCarAlongRoute(&car);
    CHECK_EQ(s_sampledIndex, -1);
    CHECK_EQ(car.headingAngle, 500);

    g_TrackPointCount = 5;
    SteerCarAlongRoute(NULL);
    CHECK_EQ(s_sampledIndex, -1);

    s_coords[0] = INT_MAX;
    s_coords[2] = INT_MIN;
    s_smoothAngle = 0;
    s_atanResult = INT_MIN;
    CHECK_EQ(CalculateTrackOffsetHeading(0, 0, INT_MIN, INT_MAX, 0),
             INT_MIN + ANGLE_QUARTER_TURN);
    CHECK_EQ(s_atanX, -1);
    CHECK_EQ(s_atanZ, 1);

    puts("route steering preserves direction, road limits, and airborne yaw");
    return 0;
}
