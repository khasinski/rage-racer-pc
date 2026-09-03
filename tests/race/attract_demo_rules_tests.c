#include <assert.h>
#include <limits.h>

#include "game/race.h"
#include "game/race_internal.h"

static void TestTitleFade(void) {
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, 0, 80, 12) == 0);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, 3, 80, 12) == 0);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, 4, 80, 12) == 4);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, 100, 80, 12) == 0x7F);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_RACE, 0, -1, 0) == 0);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_RACE, 0, 42, 0) == 42);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_RACE, 0, 128, 0) == 0x7F);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, INT_MAX, 0,
                                INT_MIN) == 0x7F);
    assert(AttractTitleFadeLevel(ATTRACT_DEMO_STEP_LOAD, INT_MIN, 0,
                                INT_MAX) == 0);
}

static void TestRaceFadeSchedule(void) {
    assert(AttractOpeningWashLevel(6) == 255);
    assert(AttractOpeningWashLevel(7) == 244);
    assert(AttractOpeningWashLevel(29) == 2);
    assert(!ShouldStartAttractExitFade(0x6CB));
    assert(ShouldStartAttractExitFade(0x6CC));
    assert(!ShouldStartAttractExitFade(0x6CD));
    assert(AttractClosingWashLevel(0x6CC) == 0);
    assert(AttractClosingWashLevel(0x6CD) == 5);
    assert(!ShouldReturnFromAttractDemo(0x707));
    assert(ShouldReturnFromAttractDemo(0x708));
    assert(ShouldReturnFromAttractDemo(INT_MAX));
    assert(AttractOpeningWashLevel(INT_MIN) == INT_MAX);
    assert(AttractClosingWashLevel(INT_MAX) == INT_MAX);
}

static void TestTimers(void) {
    assert(NextAttractLoadTimer(-1) == 0);
    assert(NextAttractLoadTimer(0) == 1);
    assert(NextAttractLoadTimer(9999) == 10000);
    assert(NextAttractLoadTimer(INT_MAX) == 10000);
    assert(NextAttractRaceTimer(-1) == 0);
    assert(NextAttractRaceTimer(0x707) == 0x708);
    assert(NextAttractRaceTimer(INT_MAX) == 0x708);
}

int main(void) {
    TestTitleFade();
    TestRaceFadeSchedule();
    TestTimers();
    return 0;
}
