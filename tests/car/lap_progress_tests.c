#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

s32 g_EngineRpm;
s16 g_PeakOutputRpm;
s16 g_PeakOutputValue;
s16 g_StandingStartState;
s32 g_StandingStartSpin;
s16 g_GripLossTimer;
s32 g_RaceSeries;
s32 g_TrackPointCount;
GameCarSpec *g_CarSpec;
GameTrackPoint *g_TrackPoints;
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
    static GameCarSpec spec;
    PlayerCarRuntime player;
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
    g_CarSpec = &spec;
    events.trackWalkStart = 1;

    memset(&player, 0, sizeof(player));
    spec.revLimit = 8000;
    g_EngineRpm = 4000;
    g_PeakOutputRpm = 3000;
    g_PeakOutputValue = 1000;
    player.drive.gear = 2;
    player.drive.drivetrainTorque = 600;
    BeginCarStandingStart(&player);
    CHECK_EQ(player.drive.drivetrainTorque, 300);
    CHECK_EQ(g_StandingStartSpin, 1250);
    CHECK_EQ(g_GripLossTimer, 200);

    player.drive.gear = 0;
    player.drive.drivetrainTorque = 600;
    spec.revLimit = 0;
    BeginCarStandingStart(&player);
    CHECK_EQ(player.drive.gear, 1);
    CHECK_EQ(player.drive.drivetrainTorque, 600);

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

    g_TrackPointCount = 0;
    ResetCar(&car, 2, 77);
    SeedCarLapProgress(&car, 0);
    CHECK_EQ(car.progressA, 0);
    car.activeFlag = 0;
    AccumulateLapProgress(&car);
    CHECK_EQ(car.activeFlag, -1);

    puts("lap progress preserves seed directions, shortest paths, and ties");
    return 0;
}
