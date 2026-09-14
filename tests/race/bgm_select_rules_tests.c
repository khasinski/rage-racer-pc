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

int main(void) {
    TestSceneTimer();
    return 0;
}
