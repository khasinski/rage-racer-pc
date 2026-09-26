#include "game/race_sim.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#define UNCHANGED(call) do { const RaceSim saved = race; CHECK(!(call)); CHECK(memcmp(&race, &saved, sizeof(race)) == 0); } while (0)

int main(void) {
    /* Lifecycle guards do not need a loaded track or a prepared engine. */
    RaceSim race = {0};
    CHECK(!StartRaceSim(NULL, 0));
    CHECK(!StepRaceSim(NULL));
    CHECK(!RetireRaceDriver(NULL, 0));
    UNCHANGED(StartRaceSim(&race, 0));
    UNCHANGED(StepRaceSim(&race));
    /* Missing route is rejected before dereferencing car setup or resetting a
     * seat. All other setup arguments are real stack objects, not mocks. */
    const GameCarSpec spec = {0};
    const CarHullPoint points[PLAYER_HULL_SAMPLE_COUNT] = {0};
    const CarHullPoint corners[CAR_HULL_CORNER_COUNT] = {0};
    const DriverHull hull = {points, corners};
    const LaunchSpeedThreshold threshold = {0};
    const TrackRivalStart position = {0};
    race.route.count = 1;
    race.route.length = 100;
    UNCHANGED(AddRaceDriver(&race, 0, &spec, &hull, corners, &threshold,
                            &position, 0, 1, 0, 1));
    const GameTrackPoint point = {.segmentLength = 100};
    race.route.points = &point;
    race.route.length = 0;
    UNCHANGED(AddRaceDriver(&race, 0, &spec, &hull, corners, &threshold,
                            &position, 0, 1, 0, 1));
    race.route.length = 100;
    race.route.count = -1;
    UNCHANGED(AddRaceDriver(&race, 0, &spec, &hull, corners, &threshold,
                            &position, 0, 1, 0, 1));
    race.route = (TrackRoute){0};
    race.drivers[4].status = SIM_DRIVING;
    CHECK(StartRaceSim(&race, 37));
    CHECK(race.phase == SIM_COUNTDOWN && race.countdown == 37);
    UNCHANGED(StartRaceSim(&race, 0));
    UNCHANGED(RetireRaceDriver(&race, -1));
    UNCHANGED(RetireRaceDriver(&race, DRIVER_SEAT_LIMIT));
    UNCHANGED(RetireRaceDriver(&race, 0));
    const SimDriver seat = race.drivers[4];
    CHECK(RetireRaceDriver(&race, 4));
    CHECK(race.drivers[4].status == SIM_RETIRED);
    CHECK(race.drivers[4].car.activeFlag == -1);
    SimDriver expected = seat;
    expected.status = SIM_RETIRED;
    expected.car.activeFlag = -1;
    CHECK(memcmp(&race.drivers[4], &expected, sizeof(expected)) == 0);
    UNCHANGED(RetireRaceDriver(&race, 4));
    CHECK(StepRaceSim(&race));
    CHECK(race.phase == SIM_FINISHED && race.tick == 1);
    CHECK(race.countdown == 37 && race.elapsed == 0 && race.finishCount == 0);
    UNCHANGED(StepRaceSim(&race));
    UNCHANGED(StartRaceSim(&race, 0));

    race = (RaceSim){0};
    race.drivers[4].status = SIM_DRIVING;
    CHECK(StartRaceSim(&race, 0));
    CHECK(race.phase == SIM_RACING);
    race.tick = UINT32_MAX;
    UNCHANGED(StepRaceSim(&race));
    race.tick = 12;
    race.elapsed = UINT32_MAX;
    UNCHANGED(StepRaceSim(&race));
    /* An inactive car cannot start or receive countdown engine updates. */
    RaceSim inactive = {.phase = SIM_SETUP};
    inactive.drivers[5].status = SIM_DRIVING;
    inactive.drivers[5].car.activeFlag = -1;
    const RaceSim beforeStart = inactive;
    CHECK(!StartRaceSim(&inactive, 50));
    CHECK(memcmp(&inactive, &beforeStart, sizeof(inactive)) == 0);
    inactive.phase = SIM_COUNTDOWN;
    inactive.countdown = 37;
    inactive.tick = 1; /* next tick would normally update countdown engines */
    const PlayerCarRuntime stopped = inactive.drivers[5].car;
    CHECK(StepRaceSim(&inactive));
    CHECK(inactive.phase == SIM_FINISHED && inactive.countdown == 37);
    CHECK(inactive.drivers[5].status == SIM_RETIRED && inactive.elapsed == 0);
    CHECK(memcmp(&inactive.drivers[5].car, &stopped, sizeof(stopped)) == 0);
    /* Finishing/retiring one seat must not rewrite another seat's result. */
    race.drivers[2].status = SIM_DRIVER_FINISHED;
    race.drivers[2].place = 1;
    race.drivers[2].finishTick = 99;
    const SimDriver finished = race.drivers[2];
    CHECK(RetireRaceDriver(&race, 4));
    CHECK(memcmp(&race.drivers[2], &finished, sizeof(finished)) == 0);
    UNCHANGED(RetireRaceDriver(&race, 2));
    return 0;
}
