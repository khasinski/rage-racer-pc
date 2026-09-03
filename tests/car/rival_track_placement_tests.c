#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];

static s32 s_events[16];
static s32 s_eventCount;

static s32 CarIndex(const GameCarRuntime *car) {
    return (s32)(car - g_Cars);
}

void AccumulateLapProgress(GameCarRuntime *car) {
    s_events[s_eventCount++] = 100 + CarIndex(car);
}

void ApplyCarKnockback(GameCarRuntime *car) {
    s_events[s_eventCount++] = 200 + CarIndex(car);
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 point,
                        const CarTrackLimits *limits) {
    if (point != car->trackPointIndex || limits->leftInset != -0x3C ||
        limits->rightInset != 0x3C || limits->leftContact != 0 ||
        limits->rightContact != 0) {
        return -1;
    }
    s_events[s_eventCount++] = 300 + CarIndex(car);
    return 0;
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    static const s32 expected[] = {
        100, 102, 103,
        200, 300, 302, 203, 303,
    };
    s32 index;

    memset(g_Cars, 0, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].activeFlag = -1;
        g_Cars[index].trackPointIndex = index + 10;
    }
    g_Cars[0].activeFlag = 0;
    g_Cars[0].motionActive = 1;
    g_Cars[0].motionTimer = 1;
    g_Cars[2].activeFlag = 0;
    g_Cars[2].motionTimer = 0;
    g_Cars[3].activeFlag = 0;
    g_Cars[3].motionActive = 1;
    g_Cars[3].motionTimer = 0x8000;

    PlaceRivalCarsOnTrack();

    CHECK_EQ(s_eventCount, (s32)(sizeof(expected) / sizeof(expected[0])));
    for (index = 0; index < s_eventCount; index++) {
        CHECK_EQ(s_events[index], expected[index]);
    }
    puts("rival placement preserves two-pass ordering and knockback timers");
    return 0;
}
