#include "game/rival.h"
#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    const GameTrackPoint points[2] = {
        {.x = 0, .y = 50, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .y = 50, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute route = {.points = points, .count = 2, .length = 2000};
    const CarTrackLimits limits = {.rightInset = 60, .leftInset = -60};
    for (int reverse = 0; reverse < 2; reverse++) {
        GameCarRuntime car = {.x = 200, .motionActive = 1, .motionTimer = 3,
            .velocityX = 10, .velocityZ = 5};
        GameCarRuntime expected = car;
        ApplyCarKnockback(&expected);
        StepCarTrackState(&expected, &route, expected.trackPointIndex, &limits, reverse, 0);
        PlaceRival(&car, &route, reverse);
        CHECK(memcmp(&car, &expected, sizeof(car)) == 0);
        CHECK(car.y == 50 && car.motionTimer == 2);
        GameCarRuntime restored = car;
        PlaceRival(&car, &route, reverse);
        PlaceRival(&restored, &route, reverse);
        CHECK(memcmp(&car, &restored, sizeof(car)) == 0);
        const GameCarRuntime saved = car;
        PlaceRival(&car, NULL, reverse);
        TrackRoute invalid = route;
        invalid.length = 0;
        PlaceRival(&car, &invalid, reverse);
        PlaceRival(NULL, &route, reverse);
        CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
        car.activeFlag = -1;
        restored = car;
        PlaceRival(&car, &route, reverse);
        CHECK(memcmp(&car, &restored, sizeof(car)) == 0);
    }
    return 0;
}
