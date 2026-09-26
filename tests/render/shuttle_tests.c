#include "game/shuttle_scenery.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    ShuttleConfig retail;
    CHECK(RetailShuttle(0, &retail));
    CHECK(retail.path.endpoint[0].x == 20908 && retail.path.endpoint[1].x == 27399);
    CHECK(retail.travel == 628 && retail.dwell == 300);
    const ShuttleConfig savedConfig = retail;
    CHECK(!RetailShuttle(-1, &retail));
    CHECK(!RetailShuttle(SHUTTLE_PATH_COUNT, &retail));
    CHECK(!RetailShuttle(0, NULL));
    CHECK(memcmp(&savedConfig, &retail, sizeof(retail)) == 0);
    const ShuttlePath path = {{{0, 20, 40, 7}, {100, 120, 140, 9}}};
    const ShuttlePath other = {{{500, 600, 700, 8}, {1000, 1100, 1200, 10}}};
    const SVec angles = {1, 2, 3, 0};
    GameShuttleScenery first, second;
    CHECK(InitShuttle(&first, &path, &angles, 0, 1));
    CHECK(InitShuttle(&second, &other, &angles, 1, 0));
    CHECK(first.position.x == 0 && first.position.w == 7 && first.angleZ == 3);
    CHECK(StepShuttle(&first, &path, 2, 1));
    CHECK(first.position.x == 0 && first.travelStep == 1);
    CHECK(StepShuttle(&first, &path, 2, 1));
    CHECK(first.position.x == 50 && first.travelStep == 2);
    CHECK(StepShuttle(&first, &path, 2, 1));
    CHECK(first.position.x == 100 && first.startEndpoint == 1 && first.dwellCounter == 0);
    CHECK(StepShuttle(&first, &path, 2, 1));
    CHECK(first.position.x == 100 && first.travelStep == 0 && first.dwellCounter == 1);
    GameShuttleScenery soloFirst = first, soloSecond = second;
    for (unsigned i = 0; i < 1000; ++i) {
        CHECK(StepShuttle(&first, &path, 2, 1));
        CHECK(StepShuttle(&second, &other, 3, 0));
    }
    for (unsigned i = 0; i < 1000; ++i) CHECK(StepShuttle(&soloFirst, &path, 2, 1));
    for (unsigned i = 0; i < 1000; ++i) CHECK(StepShuttle(&soloSecond, &other, 3, 0));
    CHECK(memcmp(&first, &soloFirst, sizeof(first)) == 0);
    CHECK(memcmp(&second, &soloSecond, sizeof(second)) == 0);
    const GameShuttleScenery saved = first;
    CHECK(!StepShuttle(&first, &path, 0, 1));
    CHECK(!StepShuttle(&first, NULL, 2, 1));
    CHECK(!InitShuttle(&first, &path, &angles, 3, 1));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    first.startEndpoint = -1;
    const GameShuttleScenery invalid = first;
    CHECK(!StepShuttle(&first, &path, 2, 1));
    CHECK(memcmp(&first, &invalid, sizeof(first)) == 0);
    const ShuttlePath extremes = {{{INT_MAX, 0, 0, 0}, {INT_MIN, 0, 0, 0}}};
    CHECK(InitShuttle(&first, &extremes, &angles, 0, 0));
    first.travelStep = 1;
    CHECK(StepShuttle(&first, &extremes, 2, 0));
    CHECK(first.position.x == 0); /* Wrapped sum -1, then truncating division. */
    return 0;
}
