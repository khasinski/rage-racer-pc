#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    TrackAiSpeedKey table[TRACK_AI_SPEED_KEY_COUNT] = {0};
    table[0].progress = 64; table[1].progress = 128;
    table[0].slotTargetSpeeds[0] = 100; table[1].slotTargetSpeeds[0] = 200;
    table[0].slotTargetSpeeds[3] = table[1].slotTargetSpeeds[3] = 400;
    GameCarRuntime car = {.trackProgress = 96 * 16};
    StepRivalTargetSpeed(&car, 0, table, 0);
    CHECK(car.accelerationLimit == 65 && car.speedKeyIndex == 0);
    GameCarRuntime saved = car;
    StepRivalTargetSpeed(&car, 5, table, 1);
    CHECK(car.accelerationLimit == 140); /* Fourth column, reduced to 80 percent. */
    car.trackProgress = 129 * 16;
    StepRivalTargetSpeed(&car, 0, table, 0);
    CHECK(car.speedKeyIndex == 1 && car.accelerationLimit == 140);
    car = saved;
    car.speedKeyIndex = TRACK_AI_SPEED_KEY_COUNT - 1;
    StepRivalTargetSpeed(&car, 0, table, 0);
    CHECK(car.speedKeyIndex == 0 && car.accelerationLimit == 65);
    /* Lap reset retains the previously selected pair for this step. */
    table[1].progress = 0; table[2].progress = 32;
    table[1].slotTargetSpeeds[0] = table[2].slotTargetSpeeds[0] = 300;
    car.trackProgress = 16 * 16; car.speedKeyIndex = 1;
    StepRivalTargetSpeed(&car, 0, table, 0);
    CHECK(car.speedKeyIndex == 0 && car.accelerationLimit == 131);
    saved = car;
    StepRivalTargetSpeed(&car, -1, table, 0);
    StepRivalTargetSpeed(&car, RACE_CAR_SLOT_COUNT, table, 0);
    StepRivalTargetSpeed(&car, 0, NULL, 0);
    StepRivalTargetSpeed(NULL, 0, table, 0);
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    GameCarRuntime restored = car;
    StepRivalTargetSpeed(&car, 0, table, 0);
    StepRivalTargetSpeed(&restored, 0, table, 0);
    CHECK(memcmp(&car, &restored, sizeof(car)) == 0);
    return 0;
}
