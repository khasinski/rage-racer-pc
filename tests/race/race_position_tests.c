#include "game/race_sim.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    RaceSim race = {0};
    CHECK(RacePosition(NULL, 0) == 0);
    CHECK(RacePosition(&race, -1) == 0 && RacePosition(&race, DRIVER_SEAT_LIMIT) == 0);
    CHECK(RacePosition(&race, 0) == 0);
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        race.drivers[seat].status = SIM_DRIVING;
        race.drivers[seat].rival = seat & 1;
    }
    RaceSim before = race;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat)
        CHECK(RacePosition(&race, seat) == seat + 1);
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    race.drivers[11].car.progressA = INT_MAX;
    race.drivers[11].car.progressB = INT_MAX;
    CHECK(RacePosition(&race, 11) == 1); /* Wide sum must not wrap behind. */
    race.drivers[0].car.progressA = INT_MIN;
    race.drivers[0].car.progressB = INT_MIN;
    CHECK(RacePosition(&race, 0) == 12);
    race.drivers[3].status = SIM_RETIRED;
    race.drivers[4].car.activeFlag = -1;
    CHECK(RacePosition(&race, 3) == 0 && RacePosition(&race, 4) == 0);
    CHECK(RacePosition(&race, 0) == 10);
    race.drivers[6].status = SIM_DRIVER_FINISHED;
    race.drivers[6].car.activeFlag = -1;
    race.drivers[6].car.progressA = INT_MIN;
    race.drivers[6].place = 1;
    CHECK(RacePosition(&race, 6) == 1 && RacePosition(&race, 11) == 2);
    before = race;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) (void)RacePosition(&race, seat);
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    return 0;
}
