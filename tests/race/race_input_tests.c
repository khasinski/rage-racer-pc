#include "game/race_sim.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    /* Input acceptance needs a seat and lifecycle state, not a physics world. */
    RaceSim race = {.phase = SIM_RACING, .tick = 37};
    race.drivers[3].status = SIM_DRIVING;
    const DriverInput invalid[] = {
        {.throttle = -1}, {.throttle = 257}, {.brake = -1}, {.brake = 257},
        {.steering.mode = -1}, {.steering.mode = STEERING_AUTOMATIC},
        {.steering.angle = -13 * 512 - 1}, {.steering.angle = 13 * 512 + 1},
        {.steering.left = -1}, {.steering.left = 2},
        {.steering.right = -1}, {.steering.right = 2},
        {.shiftUp = -1}, {.shiftUp = 2}, {.shiftDown = -1}, {.shiftDown = 2},
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        const RaceSim before = race;
        CHECK(!ValidDriverInput(&invalid[i]));
        CHECK(!SetRaceInput(&race, 3, &invalid[i]));
        CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    }
    DriverInput input = {.steering = {.mode = STEERING_ANALOG,
        .angle = -13 * 512, .left = 1, .right = 1},
        .throttle = 256, .brake = 256, .shiftUp = 1};
    CHECK(!ValidDriverInput(NULL));
    CHECK(ValidDriverInput(&input));
    CHECK(SetRaceInput(&race, 3, &input));
    CHECK(memcmp(&race.drivers[3].input, &input, sizeof(input)) == 0);
    input = (DriverInput){.steering = {.mode = STEERING_DIGITAL,
        .angle = 13 * 512}, .shiftDown = 1};
    CHECK(SetRaceInput(&race, 3, &input));
    CHECK(race.drivers[3].input.shiftUp == 1 && race.drivers[3].input.shiftDown == 1);
    CHECK(race.drivers[3].input.throttle == 0 && race.drivers[3].input.brake == 0);
    input = (DriverInput){0};
    CHECK(SetRaceInput(&race, 3, &input));
    CHECK(race.drivers[3].input.steering.mode == STEERING_CENTER);
    CHECK(race.drivers[3].input.shiftUp == 1 && race.drivers[3].input.shiftDown == 1);
    const RaceSim before = race;
    CHECK(!SetRaceInput(NULL, 3, &input));
    CHECK(!SetRaceInput(&race, 3, NULL));
    CHECK(!SetRaceInput(&race, -1, &input));
    CHECK(!SetRaceInput(&race, DRIVER_SEAT_LIMIT, &input));
    CHECK(!SetRaceInput(&race, 0, &input));
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    race.drivers[3].rival = 1;
    CHECK(!SetRaceInput(&race, 3, &input));
    race.drivers[3].rival = 0;
    race.drivers[3].car.activeFlag = -1;
    const RaceSim inactive = race;
    CHECK(!SetRaceInput(&race, 3, &input));
    CHECK(memcmp(&race, &inactive, sizeof(race)) == 0);
    race.drivers[3].car.activeFlag = 0;
    for (int status = SIM_EMPTY; status <= SIM_RETIRED; status++) {
        if (status == SIM_DRIVING) continue;
        race.drivers[3].status = status;
        const RaceSim unchanged = race;
        CHECK(!SetRaceInput(&race, 3, &input));
        CHECK(memcmp(&race, &unchanged, sizeof(race)) == 0);
    }
    race.drivers[3].status = SIM_DRIVING;
    race.phase = SIM_FINISHED;
    CHECK(!SetRaceInput(&race, 3, &input));
    return 0;
}
