#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
s32 g_TrackLength;
const TrackEventData *g_TrackEventData;

static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

static void CheckConfiguredRival(void) {
    TrackEventData events;
    RaceGridSlot grid[7];
    GameCarRuntime car;
    TrackRivalAiConfig *config;

    memset(&events, 0, sizeof(events));
    memset(grid, 0, sizeof(grid));
    memset(&car, 0, sizeof(car));
    g_TrackEventData = &events;
    g_RaceSeries = 1;
    g_TrackLength = 12000;
    grid[6].value = 5;
    config = &events.rivalAiConfigs[1][5];
    config->speed = 160;
    config->accelerationStep = 7;
    config->boostAccelerationThreshold = 11;
    config->collisionBoostDuration = 0xFFFF;
    config->boostAcceleration = 16;
    config->minimumSpeed = 59;
    config->initialEngineRpm = 0xFFFF;
    car.boostTimer = 123;
    car.engineRpm = (s32)0x5A5AFFFF;

    InitRivalCarAi(&car, 6, grid);

    CHECK_EQ(car.targetSpeed, 1168, "scaled target speed");
    CHECK_EQ(car.accelerationStep, 7, "acceleration step");
    CHECK_EQ(car.boostAccelerationThreshold, 10, "boost threshold clamp");
    CHECK_EQ(car.collisionBoostDuration, 0, "negative boost duration");
    CHECK_EQ(car.boostAcceleration, 15, "boost acceleration clamp");
    CHECK_EQ(car.boostTimer, 0, "boost timer reset");
    CHECK_EQ(car.minimumSpeed, 60, "minimum speed clamp");
    CHECK_EQ((u16)car.engineRpm, 0, "negative initial rpm");
    CHECK_EQ((u32)car.engineRpm >> 16, 0x5A5A,
             "initial rpm preserves upper halfword");
    CHECK_EQ(car.accelerationLimit, 70, "acceleration limit");
    CHECK_EQ(car.gridTargetProgress, 1600, "late grid target");
}

static void CheckInvalidConfigIndex(void) {
    TrackEventData events;
    RaceGridSlot grid = {.value = -1};
    GameCarRuntime car;

    memset(&events, 0, sizeof(events));
    memset(&car, 0, sizeof(car));
    g_TrackEventData = &events;
    g_RaceSeries = 0;
    g_TrackLength = 12000;
    events.rivalAiConfigs[0][0].speed = 80;
    events.rivalAiConfigs[0][0].minimumSpeed = 100;

    InitRivalCarAi(&car, 0, &grid);

    CHECK_EQ(car.targetSpeed, 584, "invalid index uses first config");
    CHECK_EQ(car.gridTargetProgress, 1000, "front grid target");
}

static void CheckNegativeMotionConfig(void) {
    TrackEventData events;
    RaceGridSlot grid = {0};
    GameCarRuntime car;
    TrackRivalAiConfig *config;

    memset(&events, 0, sizeof(events));
    memset(&car, 0, sizeof(car));
    g_TrackEventData = &events;
    g_RaceSeries = 0;
    g_TrackLength = 12000;
    config = &events.rivalAiConfigs[0][0];
    config->speed = -1;
    config->accelerationStep = 0xFFFF;

    InitRivalCarAi(&car, 0, &grid);

    CHECK_EQ(car.targetSpeed, 0, "negative target speed");
    CHECK_EQ(car.accelerationStep, 0, "negative acceleration step");
    CHECK_EQ(car.accelerationLimit, 0, "negative speed acceleration limit");

    config->speed = INT16_MIN;
    config->accelerationStep = 0x8000;
    config->minimumSpeed = 0x8000;
    config->initialEngineRpm = 0x8000;
    InitRivalCarAi(&car, 0, &grid);
    CHECK_EQ(car.targetSpeed, 0, "minimum encoded target speed");
    CHECK_EQ(car.accelerationStep, 0, "minimum encoded acceleration step");
    CHECK_EQ(car.minimumSpeed, 60, "minimum encoded speed uses lower bound");
    CHECK_EQ((u16)car.engineRpm, 0,
             "minimum encoded engine rpm uses lower bound");

    config->speed = INT16_MAX;
    InitRivalCarAi(&car, 0, &grid);
    CHECK_EQ(car.targetSpeed, -22945,
             "maximum encoded target speed keeps its low halfword");
    CHECK_EQ(car.accelerationLimit, -1376,
             "wrapped target speed feeds acceleration limit");
}

int main(void) {
    CheckConfiguredRival();
    CheckInvalidConfigIndex();
    CheckNegativeMotionConfig();
    if (s_failures != 0) {
        return 1;
    }
    puts("rival car initialization passed");
    return 0;
}
