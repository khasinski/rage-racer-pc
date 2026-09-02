#include <assert.h>

#include "game/race_internal.h"

static void TestEndingWashThreshold(void) {
    assert(!ReplayEndingWashActive(0, 599));
    assert(!ReplayEndingWashActive(399, 1000));
    assert(!ReplayEndingWashActive(400, 1000));
    assert(ReplayEndingWashActive(401, 1000));
    assert(ReplayEndingWashLevel(400, 1000) == 0);
    assert(ReplayEndingWashLevel(401, 1000) == 1);
    assert(ReplayEndingWashLevel(500, 1000) == 100);
    assert(ReplayEndingWashLevel(1000, 1000) == 255);
}

static void TestAutomaticExitFadeThreshold(void) {
    assert(!ShouldStartReplayExitFade(0, 67));
    assert(ShouldStartReplayExitFade(0, 68));
    assert(!ShouldStartReplayExitFade(931, 1000));
    assert(ShouldStartReplayExitFade(932, 1000));
    assert(!ShouldStartReplayExitFade(933, 1000));
}

static void TestReplayBadgeBlink(void) {
    assert(!ReplayBadgeVisible(15, 0));
    assert(ReplayBadgeVisible(16, 0));
    assert(ReplayBadgeVisible(31, 0));
    assert(!ReplayBadgeVisible(32, 0));
    assert(!ReplayBadgeVisible(16, 1));
}

static void TestReplayResultCue(void) {
    assert(ReplayResultCue(59, 1, 0, 1) == -1);
    assert(ReplayResultCue(60, 0, 0, 1) == -1);
    assert(ReplayResultCue(60, 1, 1, 1) == -1);
    assert(ReplayResultCue(60, 1, 0, 1) == 0x40);
    assert(ReplayResultCue(60, 1, 0, 2) == 0x41);
    assert(ReplayResultCue(60, 2, 0, 12) == 0x41);
    assert(ReplayResultCue(61, 1, 0, 1) == -1);
}

static void TestReplayCursorWrap(void) {
    assert(NextReplayReadCursor(0, 100) == 1);
    assert(NextReplayReadCursor(98, 100) == 99);
    assert(NextReplayReadCursor(99, 100) == 0);
    assert(NextReplayReadCursor(100, 100) == 0);
}

int main(void) {
    TestEndingWashThreshold();
    TestAutomaticExitFadeThreshold();
    TestReplayBadgeBlink();
    TestReplayResultCue();
    TestReplayCursorWrap();
    return 0;
}
