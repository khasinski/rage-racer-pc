#include "game/race_sim.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    RaceSim race = {.phase = SIM_RACING, .laps = 3, .elapsed = 123};
    SimDriver *driver = &race.drivers[11];
    driver->status = SIM_DRIVING;
    CHECK(RaceTime(&race, 11) == 2460);
    CHECK(RaceLapTime(&race, 11, 0) == -1);
    driver->car.lap = 1;
    driver->lapStarted = 100;
    CHECK(RaceLapTime(&race, 11, 0) == 460);
    CHECK(RaceLapTime(&race, 11, 1) == -1);
    driver->car.lap = 2;
    driver->lapTicks[0] = 50;
    CHECK(RaceLapTime(&race, 11, 0) == 1000);
    CHECK(RaceLapTime(&race, 11, 1) == 460);
    driver->status = SIM_DRIVER_FINISHED;
    driver->car.activeFlag = -1;
    driver->car.lap = 4;
    driver->finishTick = 90;
    driver->lapTicks[1] = 20; driver->lapTicks[2] = 0;
    CHECK(RaceTime(&race, 11) == 1800);
    CHECK(RaceLapTime(&race, 11, 1) == 400 && RaceLapTime(&race, 11, 2) == 0);
    race.elapsed = UINT32_MAX;
    CHECK(RaceTime(&race, 11) == 1800); /* Finish time stays frozen. */
    driver->finishTick = UINT32_MAX; driver->lapTicks[0] = UINT32_MAX;
    CHECK(RaceTime(&race, 11) == INT32_MAX && RaceLapTime(&race, 11, 0) == INT32_MAX);
    RaceSim before = race;
    CHECK(RaceTime(NULL, 0) == -1 && RaceLapTime(NULL, 0, 0) == -1);
    CHECK(RaceTime(&race, -1) == -1 && RaceTime(&race, DRIVER_SEAT_LIMIT) == -1);
    CHECK(RaceLapTime(&race, 11, -1) == -1 && RaceLapTime(&race, 11, 3) == -1);
    CHECK(RaceLapTime(&race, 11, PLAYER_LAP_TIME_CAPACITY) == -1);
    CHECK(RaceTime(&race, 0) == -1 && RaceLapTime(&race, 0, 0) == -1);
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    driver->status = SIM_RETIRED;
    CHECK(RaceTime(&race, 11) == -1 && RaceLapTime(&race, 11, 0) == -1);
    driver->status = SIM_DRIVING; driver->car.activeFlag = 0; driver->car.lap = 1;
    race.elapsed = 10; driver->lapStarted = 11;
    CHECK(RaceLapTime(&race, 11, 0) == -1); /* Never unsigned-underflow. */
    driver->lapStarted = 0; race.phase = SIM_COUNTDOWN;
    CHECK(RaceLapTime(&race, 11, 0) == -1);
    return 0;
}
