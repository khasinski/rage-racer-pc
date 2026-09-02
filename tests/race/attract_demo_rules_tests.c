#include <assert.h>

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
}

static void TestCdTrackMapping(void) {
    assert(AttractDemoCdTrack(0) == 3);
    assert(AttractDemoCdTrack(8) == 11);
    assert(AttractDemoCdTrack(9) == 0x11);
    assert(AttractDemoCdTrack(10) == 13);
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
}

int main(void) {
    TestTitleFade();
    TestCdTrackMapping();
    TestRaceFadeSchedule();
    return 0;
}
