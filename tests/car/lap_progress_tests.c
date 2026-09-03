#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
s32 g_TrackPointCount;
const GameTrackPoint *g_TrackPoints;
TrackEventData *g_TrackEventData;

static s32 s_targetSegment;

s32 FindTrackSegment(GameCarRuntime *car, s32 startIndex) {
    (void)car;
    (void)startIndex;
    return s_targetSegment;
}

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
    static TrackEventData events;
    GameCarRuntime car;
    s32 i;

    memset(points, 0, sizeof(points));
    memset(&events, 0, sizeof(events));
    for (i = 0; i < 5; i++) {
        points[i].segmentLength = (u16)((i + 1) * 10);
    }
    g_TrackPoints = points;
    g_TrackPointCount = 5;
    g_TrackEventData = &events;
    events.trackWalkStart = 1;

    ResetCar(&car, 4, 999);
    g_RaceSeries = 0;
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, -120);
    SeedCarLapProgress(&car, 1);
    CHECK_EQ(car.progressA, 30);

    g_RaceSeries = 1;
    SeedCarLapProgress(&car, 1);
    CHECK_EQ(car.progressA, 70);
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, -80);

    ResetCar(&car, 1, 100);
    s_targetSegment = 4;
    g_RaceSeries = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 130);
    CHECK_EQ(car.trackPointIndex, 4);

    ResetCar(&car, 1, 100);
    g_RaceSeries = 1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 40);

    ResetCar(&car, 1, 100);
    s_targetSegment = 3;
    g_RaceSeries = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 30);

    ResetCar(&car, 1, 100);
    g_RaceSeries = 1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 150);

    g_TrackPointCount = 4;
    ResetCar(&car, 0, 0);
    s_targetSegment = 2;
    g_RaceSeries = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 50);
    ResetCar(&car, 0, 0);
    g_RaceSeries = 1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 30);

    ResetCar(&car, 2, 77);
    s_targetSegment = -1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.activeFlag, -1);
    CHECK_EQ(car.progressA, 77);

    g_TrackPointCount = 5;
    ResetCar(&car, 7, 77);
    s_targetSegment = 2;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.trackPointIndex, 2);
    CHECK_EQ(car.progressA, 77);

    ResetCar(&car, 1, 100);
    points[1].segmentLength = (u16)-1;
    s_targetSegment = 2;
    g_RaceSeries = 1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, 100);
    points[1].segmentLength = 20;

    ResetCar(&car, 1, INT_MAX);
    s_targetSegment = 2;
    g_RaceSeries = 1;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, INT_MIN + 19);

    ResetCar(&car, 1, INT_MIN);
    g_RaceSeries = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.progressA, INT_MAX - 29);

    g_TrackPointCount = 0;
    ResetCar(&car, 2, 77);
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, 0);
    car.activeFlag = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.activeFlag, -1);

    g_TrackPointCount = 5;
    g_TrackPoints = points;
    g_TrackEventData = NULL;
    ResetCar(&car, 2, 77);
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, 0);

    g_TrackPoints = NULL;
    ResetCar(&car, 2, 77);
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, 0);
    car.activeFlag = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.activeFlag, -1);

    puts("lap progress preserves seed directions, shortest paths, and ties");
    return 0;
}
