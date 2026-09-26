#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    const TrackRivalAiConfig config = {.speed = 160, .accelerationStep = 7,
        .boostAccelerationThreshold = 11, .collisionBoostDuration = 0xffff,
        .boostAcceleration = 16, .minimumSpeed = 59, .initialEngineRpm = 0xffff};
    GameCarRuntime first = {.x = 123, .boostTimer = 123, .engineRpm = 0x5a5affff};
    ConfigureRival(&first, &config, 0, 12000, 6);
    CHECK(first.targetSpeed == 1168 && first.accelerationLimit == 70);
    CHECK(first.gridTargetProgress == 1600 && first.accelerationStep == 7);
    CHECK(first.boostAccelerationThreshold == 10 && first.boostAcceleration == 15);
    CHECK(first.collisionBoostDuration == 0 && first.minimumSpeed == 60);
    CHECK(first.engineRpm == 0x5a5a0000 && first.boostTimer == 0 && first.x == 123);
    const GameCarRuntime saved = first;
    const TrackRivalAiConfig other = {.speed = 80, .minimumSpeed = 100,
        .initialEngineRpm = 1200, .collisionBoostDuration = 20};
    GameCarRuntime second = {0};
    ConfigureRival(&second, &other, 0, 24000, 0);
    CHECK(second.targetSpeed == 584 && second.gridTargetProgress == 2000);
    CHECK(second.minimumSpeed == 100 && second.engineRpm == 1200);
    CHECK(second.collisionBoostDuration == 20);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    const u16 encoded[] = {0, 10, 15, 60, 0x7fff, 0x8000, 0xffff};
    const s16 thresholds[] = {0, 10, 10, 10, 10, 0, 0};
    const s16 boosts[] = {0, 10, 15, 15, 15, 0, 0};
    const s16 minimums[] = {60, 60, 60, 60, 0x7fff, 60, 60};
    for (size_t i = 0; i < sizeof(encoded) / sizeof(encoded[0]); i++) {
        TrackRivalAiConfig edge = {.boostAccelerationThreshold = encoded[i],
            .boostAcceleration = encoded[i], .minimumSpeed = encoded[i]};
        ConfigureRival(&second, &edge, 0, 12000, 3);
        CHECK(second.boostAccelerationThreshold == thresholds[i]);
        CHECK(second.boostAcceleration == boosts[i] && second.minimumSpeed == minimums[i]);
        CHECK(second.gridTargetProgress == 1000);
    }
    TrackRivalAiConfig table[TRACK_RIVAL_COUNT] = {0};
    for (int model = 0; model < TRACK_RIVAL_COUNT; model++) {
        table[model].speed = 80 + model * 10;
        table[model].accelerationStep = model + 1;
    }
    for (int model = 0; model < TRACK_RIVAL_COUNT; model++) {
        ConfigureRival(&second, table, model, 12000, 6);
        CHECK(second.targetSpeed == (80 + model * 10) * 1168 / 160);
        CHECK(second.accelerationStep == model + 1);
        CHECK(second.gridTargetProgress == 1600);
    }
    /* Model selection is independent of the authored grid position. */
    ConfigureRival(&second, table, 5, 12000, 0);
    CHECK(second.targetSpeed == 949 && second.accelerationStep == 6);
    CHECK(second.gridTargetProgress == 1000);
    const s32 invalidModels[] = {-1, TRACK_RIVAL_COUNT, INT32_MAX, INT32_MIN};
    for (size_t i = 0; i < sizeof(invalidModels) / sizeof(invalidModels[0]); i++) {
        ConfigureRival(&second, table, invalidModels[i], 12000, 0);
        CHECK(second.targetSpeed == 584 && second.accelerationStep == 1);
    }
    const GameCarRuntime unchanged = second;
    ConfigureRival(&second, NULL, 0, 12000, 0);
    ConfigureRival(NULL, table, 0, 12000, 0);
    CHECK(memcmp(&second, &unchanged, sizeof(second)) == 0);
    return 0;
}
