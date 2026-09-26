#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .y = 50, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .y = 50, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 2000, .y = 50, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    TrackRivalStart start = {.x = 200, .trackPointIndex = 0};
    GameCarRuntime first, second;
    memset(&first, 0x5a, sizeof(first));
    CHECK(InitRival(&first, &route, &start, 0, 0, 7));
    CHECK(first.initializedFlag == 1 && first.aiEnabled == 1 && first.activeFlag == 0);
    CHECK(first.modelIndex == 7 && first.rivalModelId == 7);
    CHECK(first.y == 50 && first.modelY == 50);
    CHECK(first.bodyYaw == 3072 && first.headingAngle == 3072);
    CHECK(first.speed == 0 && first.acceleration == 0 && first.collisionFlag == 0);
    CHECK(first.initialLateralOffset == first.trackLateralOffset);
    CHECK(first.aiLateralOffset == first.trackLateralOffset);
    const GameCarRuntime saved = first;
    CHECK(InitRival(&second, &route, &start, 0, 1, 8));
    CHECK(second.bodyYaw == 1024 && second.facingBackwards == 1 && second.modelIndex == 8);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    CHECK(!InitRival(&first, NULL, &start, 0, 0, 7));
    CHECK(!InitRival(&first, &route, NULL, 0, 0, 7));
    CHECK(!InitRival(&first, &route, &start, 0, 2, 7));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    TrackEventData events = {0};
    CHECK(!AdvanceRival(&first, NULL, &events, 0, 0, NULL, 0));
    CHECK(!AdvanceRival(&first, &route, NULL, 0, 0, NULL, 0));
    CHECK(!AdvanceRival(&first, &route, &events, RACE_CAR_SLOT_COUNT, 0, NULL, 0));
    CHECK(!AdvanceRival(&first, &route, &events, 0, 2, NULL, 0));
    CHECK(!AdvanceRival(&first, &route, &events, 0, 0, NULL, 1));
    CHECK(!AdvanceRival(&first, &route, &events, 0, 0, NULL, -1));
    TrafficCar traffic = {&second, 1};
    CHECK(!AdvanceRival(&first, &route, &events, 0, 0, &traffic, RACE_CAR_SLOT_COUNT + 2));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    start.activeFlag = -1;
    CHECK(InitRival(&second, &route, &start, 0, 0, 7));
    CHECK(second.activeFlag == -1 && second.modelY == 0);
    return 0;
}
