#include "game/race_sim.h"
#include "../car/driver_fixture.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    const TrackRivalStart position = {.x = 200};
    CarHullPoint hullPoints[PLAYER_HULL_SAMPLE_COUNT] = {{-32, 64}};
    CarHullPoint corners[CAR_HULL_CORNER_COUNT] = {{-26, 96}};
    CarHullPoint road[CAR_HULL_CORNER_COUNT] = {{-15, 20}};
    const DriverHull hull = {hullPoints, corners};
    LaunchSpeedThreshold threshold = {960, 320};
    PlayerCarRuntime car;
    GameCarSpec spec;
    CarPerformance engine = {0};
    PrepareDriver(&car, &spec, &engine);
    const GameCarSpec source = spec;
    RaceSim race;
    CHECK(InitRaceSim(&race, &route, NULL, 1, 0));
    const RaceSim empty = race;
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, NULL, &threshold,
                         &position, 0, 1, 0, 1));
    TrackRivalStart invalidPosition = position;
    invalidPosition.trackPointIndex = -1;
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, road, &threshold,
                         &invalidPosition, 0, 1, 0, 1));
    invalidPosition.trackPointIndex = route.count;
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, road, &threshold,
                         &invalidPosition, 0, 1, 0, 1));
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, road, &threshold,
                         &position, 0, 2, 0, 1));
    LaunchSpeedThreshold invalidThreshold = threshold;
    invalidThreshold.initial = -1;
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, road, &invalidThreshold,
                         &position, 0, 1, 0, 1));
    invalidThreshold = threshold;
    invalidThreshold.sustain = -1;
    CHECK(!AddRaceDriver(&race, 4, &spec, &hull, road, &invalidThreshold,
                         &position, 0, 1, 0, 1));
    CHECK(memcmp(&race, &empty, sizeof(race)) == 0);
    CHECK(AddRaceDriver(&race, 4, &spec, &hull, road, &threshold,
                        &position, 0, 0, 12, 123));
    CHECK(memcmp(&spec, &source, sizeof(spec)) == 0);
    CHECK(race.drivers[4].status == SIM_DRIVING && race.drivers[4].random == 123);
    CHECK(race.drivers[4].car.modelIndex == 12 && race.drivers[4].car.drive.manual == 0);
    CHECK(memcmp(race.drivers[4].hull, hullPoints, sizeof(hullPoints)) == 0);
    CHECK(memcmp(race.drivers[4].corners, corners, sizeof(corners)) == 0);
    CHECK(memcmp(race.drivers[4].roadCorners, road, sizeof(road)) == 0);
    CHECK(race.drivers[4].threshold.initial == 960 && race.drivers[4].threshold.sustain == 320);
    const RaceSim installed = race;
    memset(&spec, 0x33, sizeof(spec));
    memset(hullPoints, 0x44, sizeof(hullPoints));
    memset(corners, 0x55, sizeof(corners));
    memset(road, 0x66, sizeof(road));
    threshold = (LaunchSpeedThreshold){0};
    CHECK(memcmp(&race, &installed, sizeof(race)) == 0);
    CHECK(!AddRaceDriver(&race, 4, &source, &hull, road, &threshold,
                         &position, 0, 1, 0, 456));
    CHECK(memcmp(&race, &installed, sizeof(race)) == 0);
    CHECK(race.route.points == points); /* immutable track remains borrowed */
    return 0;
}
