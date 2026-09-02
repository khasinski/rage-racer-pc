#include "game/car.h"
#include "game/race.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
s32 g_TrackPointCount;
GameTrackPoint *g_TrackPoints;
TrackEventData *g_TrackEventData;

static s32 s_findResult;
static s32 s_findStart;
static s32 s_seedMode;
static s32 s_trackCalls;

s32 FindTrackSegment(GameCarRuntime *car, s32 startIndex) {
    (void)car;
    s_findStart = startIndex;
    return s_findResult;
}

void SeedCarLapProgress(GameCarRuntime *car, s32 mode) {
    car->progressA = 123;
    s_seedMode = mode;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 pointIndex,
                        const CarTrackLimits *limits) {
    if (limits->leftInset != -20 || limits->rightInset != 20) {
        return -1;
    }
    car->y = 40;
    car->trackProgress = 500 + pointIndex;
    s_trackCalls++;
    return 0;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        fprintf(stderr, "line %d: check failed: %s\n", __LINE__,           \
                #condition);                                                 \
        return 1;                                                            \
    }                                                                        \
} while (0)

int main(void) {
    static TrackEventData events;
    static GameTrackPoint points[3];
    RaceGridSlot grid[2];
    GameCarRuntime car;
    TrackRivalStart *start;

    memset(&events, 0, sizeof(events));
    memset(points, 0, sizeof(points));
    memset(grid, 0, sizeof(grid));
    memset(&car, 0x5A, sizeof(car));
    g_TrackEventData = &events;
    g_TrackPoints = points;
    g_TrackPointCount = 3;
    g_RaceSeries = 1;
    grid[0].halves.modelId = 7;
    start = &events.rivalStarts[1][1];
    start->x = 1000;
    start->z = 2000;
    start->trackPointIndex = -1;
    start->modelId = 4;
    points[2].angle = 0x200;
    s_findResult = -1;
    s_trackCalls = 0;

    InitRivalCar(&car, 0, grid);

    CHECK(s_findStart == 2);
    CHECK(car.trackPointIndex == 2);
    CHECK(car.x == 1000 && car.y == 40 && car.z == 2000);
    CHECK(car.bodyYaw == 0x200 && car.headingAngle == 0x200);
    CHECK(car.baseBodyYaw == 0x200 && car.targetYaw == 0x200);
    CHECK(car.modelYaw == 0x200 && car.modelY == 40);
    CHECK(car.modelIndex == 7 && car.rivalModelId == 7);
    CHECK(car.activeFlag == 4 && car.aiEnabled == 1);
    CHECK(car.initializedFlag == 1 && car.collisionFlag == 0);
    CHECK(car.progressA == 123 && car.previousTrackProgress == 502);
    CHECK(s_seedMode == 4 && s_trackCalls == 1);
    CHECK(car.speed == 0 && car.acceleration == 0);
    CHECK(car.motionX == 0 && car.motionY == 0 && car.motionZ == 0);

    memset(&car, 0x5A, sizeof(car));
    start->modelId = -1;
    s_findResult = 1;
    s_trackCalls = 0;
    InitRivalCar(&car, 0, grid);
    CHECK(car.trackPointIndex == 1);
    CHECK(car.activeFlag == -1);
    CHECK(s_trackCalls == 0);
    CHECK(car.modelY == 0);

    puts("rival runtime initialization tests passed");
    return 0;
}
