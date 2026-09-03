#include "game/round_screen_internal.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>

static s32 s_failures;

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestRoundNumber(void) {
    const u8 firstClasses[4] = {1, 2, 0, 0xFF};
    const u8 laterClasses[4] = {1, 0, 3, 0};

    Check(DetermineGrandPrixRound(firstClasses, 0, 2) == 3,
          "unplayed course is the next early-class round");
    Check(DetermineGrandPrixRound(firstClasses, 1, 1) == 2,
          "replaying a placed course keeps its original round count");
    Check(DetermineGrandPrixRound(laterClasses, 2, 3) == 3,
          "later classes count all four courses");
    Check(DetermineGrandPrixRound(NULL, 2, 0) == 0,
          "missing course progress has no round");
    Check(DetermineGrandPrixRound(laterClasses, 2, -1) == 0 &&
              DetermineGrandPrixRound(laterClasses, 2, 4) == 0,
          "invalid course index has no round");
    Check(DetermineGrandPrixRound(laterClasses, -1, 0) == 0 &&
              DetermineGrandPrixRound(laterClasses, 6, 0) == 0,
          "invalid class index has no round");
}

static void TestSelectionWrap(void) {
    Check(WrapRoundBgmSelection(-1, 10) == 10,
          "left wraps from shuffle to the last track");
    Check(WrapRoundBgmSelection(11, 10) == 0,
          "right wraps from the last track to shuffle");
    Check(WrapRoundBgmSelection(4, 0) == 0,
          "empty track list only exposes shuffle");
    Check(ClampRoundBgmTrackCount(-1) == 0 &&
              ClampRoundBgmTrackCount(8) == 8 &&
              ClampRoundBgmTrackCount(100) == 10,
          "track count stays within the displayed name table");
    Check(WrapRoundBgmSelection(11, 100) == 0,
          "oversized track count cannot expose missing names");
}

static void TestFadeAndMirrorRules(void) {
    Check(ClampRoundScreenFade(-1) == 0,
          "negative fade is fully transparent");
    Check(ClampRoundScreenFade(0x40) == 0x40,
          "fade preserves values in range");
    Check(ClampRoundScreenFade(0x80) == 0x7F &&
              ClampRoundScreenFade(1000) == 0x7F,
          "fade saturates at the PS1 brightness limit");
    Check(RoundScreenFadeFromTimer(20, 15) == 65,
          "timer fade preserves the retail ramp");
    Check(RoundScreenFadeFromTimer(INT_MAX, INT_MIN) == 0x7F &&
              RoundScreenFadeFromTimer(INT_MIN, INT_MAX) == 0,
          "timer fade avoids signed overflow at corrupt extremes");
    Check(NextRoundScreenTimer(-1) == 0 &&
              NextRoundScreenTimer(9999) == 10000 &&
              NextRoundScreenTimer(10000) == 10000 &&
              NextRoundScreenTimer(INT_MAX) == 10000,
          "scene timer recovers and saturates at its limit");

    Check(!IsRoundMirrorMode(PAD_START | PAD_R1),
          "partial mirror chord stays in normal mode");
    Check(IsRoundMirrorMode(PAD_START | PAD_R1 | PAD_L1),
          "complete mirror chord enables mirror mode");
    Check(IsRoundMirrorMode(PAD_START | PAD_R1 | PAD_L1 | PAD_CONFIRM),
          "unrelated held buttons do not cancel mirror mode");
}

static void TestTableIndices(void) {
    Check(RoundScreenTableIndicesValid(0, 0, 1) &&
              RoundScreenTableIndicesValid(1, 5, 1),
          "Grand Prix table accepts both series and all six classes");
    Check(!RoundScreenTableIndicesValid(-1, 0, 1) &&
              !RoundScreenTableIndicesValid(2, 0, 1) &&
              !RoundScreenTableIndicesValid(0, -1, 1) &&
              !RoundScreenTableIndicesValid(0, 6, 1),
          "Grand Prix table rejects invalid series and classes");
    Check(RoundScreenTableIndicesValid(1, INT_MAX, 0),
          "time attack records do not depend on the GP class");
    Check(!RoundScreenTableIndicesValid(2, 0, 0),
          "time attack still validates its record series");
}

static void TestAssetLoadGate(void) {
    Check(IsRoundScreenAssetLoadComplete(0, 0),
          "completed asset load enters the round screen");
    Check(!IsRoundScreenAssetLoadComplete(1, 0),
          "screen asset transfer keeps waiting");
    Check(!IsRoundScreenAssetLoadComplete(2, 0),
          "voice bank transfer keeps waiting");
    Check(!IsRoundScreenAssetLoadComplete(0, 1),
          "failed asset load does not install partial data");
}

static void TestBgmChoice(void) {
    const u8 order[4] = {3, 0, 1, 2};
    const u8 invalidOrder[4] = {3, 0xFF, 1, 2};
    RoundBgmChoice choice;

    choice = ChooseRoundBgm(3, order, 4, 2);
    Check(choice.track == 2 && choice.shuffleIndex == 2,
          "manual selection preserves the shuffle cursor");

    choice = ChooseRoundBgm(10, order, 4, 2);
    Check(choice.track == 1 && choice.shuffleIndex == 3,
          "invalid manual selection wraps to the current random choice");

    choice = ChooseRoundBgm(0, order, 4, 1);
    Check(choice.track == 0 && choice.shuffleIndex == 2,
          "shuffle consumes its selected logical track");

    choice = ChooseRoundBgm(0, order, 4, 3);
    Check(choice.track == 2 && choice.shuffleIndex == 0,
          "shuffle cursor wraps after its last entry");

    choice = ChooseRoundBgm(0, invalidOrder, 4, 1);
    Check(choice.track == 0 && choice.shuffleIndex == 2,
          "invalid shuffle entry falls back to the first track");

    choice = ChooseRoundBgm(0, order, 0, 8);
    Check(choice.track == 0 && choice.shuffleIndex == 0,
          "empty shuffle is safe and deterministic");

    choice = ChooseRoundBgm(0, NULL, 4, 2);
    Check(choice.track == 0 && choice.shuffleIndex == 0,
          "missing shuffle is safe and deterministic");
}

int main(void) {
    TestRoundNumber();
    TestSelectionWrap();
    TestFadeAndMirrorRules();
    TestAssetLoadGate();
    TestTableIndices();
    TestBgmChoice();

    if (s_failures != 0) {
        return 1;
    }
    puts("round screen progression and BGM rules are stable");
    return 0;
}
