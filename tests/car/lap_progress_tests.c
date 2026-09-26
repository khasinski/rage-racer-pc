#include "common.h"
#include "game/car.h"
#include "game/car_track_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

static void ResetCar(GameCarRuntime *car, s32 point, s32 progress) {
    memset(car, 0, sizeof(*car));
    car->trackPointIndex = point;
    car->progressA = progress;
}

int main(void) {
    static GameTrackPoint points[5];
    TrackRoute route = {0};
    s32 reverse = 0;
    s32 target = 0;
    GameCarRuntime car;
    s32 i;

    memset(points, 0, sizeof(points));

    for (i = 0; i < 5; i++) {
        points[i].segmentLength = (u16)((i + 1) * 10);
    }
    route.points = points;
    route.count = 5;

    ResetCar(&car, 4, 999);
    reverse = 0;
    SeedCarTrackProgress(&car, &route, 1, 0, reverse);
    CHECK_EQ(car.progressA, -120);
    SeedCarTrackProgress(&car, &route, 1, 1, reverse);
    CHECK_EQ(car.progressA, 30);

    reverse = 1;
    SeedCarTrackProgress(&car, &route, 1, 1, reverse);
    CHECK_EQ(car.progressA, 70);
    SeedCarTrackProgress(&car, &route, 1, 0, reverse);
    CHECK_EQ(car.progressA, -80);

    ResetCar(&car, 1, 100);
    target = 4;
    reverse = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 130);
    CHECK_EQ(car.trackPointIndex, 4);

    ResetCar(&car, 1, 100);
    reverse = 1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 40);

    ResetCar(&car, 1, 100);
    target = 3;
    reverse = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 30);

    ResetCar(&car, 1, 100);
    reverse = 1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 150);

    route.count = 4;
    ResetCar(&car, 0, 0);
    target = 2;
    reverse = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 50);
    ResetCar(&car, 0, 0);
    reverse = 1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 30);

    ResetCar(&car, 2, 77);
    target = -1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.activeFlag, -1);
    CHECK_EQ(car.progressA, 77);

    route.count = 5;
    ResetCar(&car, 7, 77);
    target = 2;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.trackPointIndex, 2);
    CHECK_EQ(car.progressA, 77);

    ResetCar(&car, 1, 100);
    points[1].segmentLength = (u16)-1;
    target = 2;
    reverse = 1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, 100);
    points[1].segmentLength = 20;

    ResetCar(&car, 1, INT_MAX);
    target = 2;
    reverse = 1;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, INT_MIN + 19);

    ResetCar(&car, 1, INT_MIN);
    reverse = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.progressA, INT_MAX - 29);

    route.count = 0;
    ResetCar(&car, 2, 77);
    SeedCarTrackProgress(&car, &route, 1, 0, reverse);
    CHECK_EQ(car.progressA, 0);
    car.activeFlag = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.activeFlag, -1);

    route.count = 5;
    route.points = points;
    ResetCar(&car, 2, 77);
    SeedCarTrackProgress(&car, NULL, 1, 0, reverse);
    CHECK_EQ(car.progressA, 0);

    route.points = NULL;
    ResetCar(&car, 2, 77);
    SeedCarTrackProgress(&car, &route, 1, 0, reverse);
    CHECK_EQ(car.progressA, 0);
    car.activeFlag = 0;
    MoveCarTrackProgress(&car, &route, target, reverse);
    CHECK_EQ(car.activeFlag, -1);

    GameTrackPoint secondPoints[3] = {{.segmentLength = 100},
                                    {.segmentLength = 200},
                                    {.segmentLength = 300}};
    const TrackRoute other = {.points = secondPoints, .count = 3};
    route.points = points;
    GameCarRuntime first;
    GameCarRuntime second;
    ResetCar(&first, 0, 0);
    ResetCar(&second, 0, 0);
    MoveCarTrackProgress(&first, &route, 1, 1);
    MoveCarTrackProgress(&second, &other, 1, 0);
    CHECK_EQ(first.progressA, 10);
    CHECK_EQ(second.progressA, -200);
    MoveCarTrackProgress(&first, &route, 2, 1);
    CHECK_EQ(first.progressA, 30);
    CHECK_EQ(second.trackPointIndex, 1);
    SeedCarTrackProgress(&second, &other, 0, 1, 1);
    CHECK_EQ(second.progressA, 0);
    CHECK_EQ(first.progressA, 30);
    SeedCarTrackProgress(NULL, &other, 0, 1, 1);
    MoveCarTrackProgress(NULL, &other, 0, 1);

    puts("lap progress preserves seed directions, shortest paths, and ties");
    return 0;
}
