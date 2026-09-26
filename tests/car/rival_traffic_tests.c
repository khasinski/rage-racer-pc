#include "game/rival.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
#include <string.h>

int main(void) {
    GameCarRuntime ai = {0}, humans[2] = {{0}}, saved[2];
    TrafficCar field[] = {{&ai, 0}, {&humans[0], 1}, {&humans[1], 1}, {NULL, 0}};
    ai.speed = 100;
    ai.accelerationLimit = 1000;
    humans[0].trackProgress = 100;
    humans[1].trackProgress = 200;
    humans[1].trackLateralOffset = 100;
    memcpy(saved, humans, sizeof saved);
    AvoidRivalTraffic(&ai, 0, 32768, field, 4);
    CHECK(ai.nearbyCarCount == 2);
    CHECK(ai.avoidanceActive == 1);
    CHECK(ai.avoidanceStep < 0);
    CHECK(ai.accelerationLimit == 300);
    CHECK(memcmp(saved, humans, sizeof saved) == 0);
    GameCarRuntime restored = ai;
    AvoidRivalTraffic(&ai, 0, 32768, field, 4);
    field[0].car = &restored;
    AvoidRivalTraffic(&restored, 0, 32768, field, 4);
    CHECK(memcmp(&ai, &restored, sizeof ai) == 0);
    humans[0].activeFlag = -1;
    humans[1].activeFlag = -1;
    AvoidRivalTraffic(&ai, 0, 32768, field + 1, 3);
    CHECK(ai.nearbyCarCount == 0 && ai.avoidanceActive == 0);
    AvoidRivalTraffic(&ai, 0, 0, field, 4);
    CHECK(ai.avoidanceStep == 0);
    AvoidRivalTraffic(NULL, 0, 32768, field, 4);
    return 0;
}
