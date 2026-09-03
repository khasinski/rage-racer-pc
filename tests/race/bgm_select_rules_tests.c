#include <assert.h>
#include <limits.h>

#include "game/race_internal.h"

static void TestSceneTimer(void) {
    assert(NextBgmSelectTimer(-1) == 0);
    assert(NextBgmSelectTimer(0) == 1);
    assert(NextBgmSelectTimer(9999) == 10000);
    assert(NextBgmSelectTimer(10000) == 10000);
    assert(NextBgmSelectTimer(INT_MAX) == 10000);
}

static void TestFadeStep(void) {
    assert(StepBgmSelectFade(100, 4, 257) == 104);
    assert(StepBgmSelectFade(2, -4, 257) == 0);
    assert(StepBgmSelectFade(254, 4, 257) == 257);
    assert(StepBgmSelectFade(INT_MAX, INT_MAX, 257) == 257);
    assert(StepBgmSelectFade(INT_MIN, INT_MIN, 257) == 0);
    assert(StepBgmSelectFade(10, 1, -1) == 0);
}

int main(void) {
    TestSceneTimer();
    TestFadeStep();
    return 0;
}
