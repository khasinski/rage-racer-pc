#include "game/round_screen_internal.h"
#include "game/state.h"

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
}

static void TestSelectionWrap(void) {
    Check(WrapRoundBgmSelection(-1, 10) == 10,
          "left wraps from shuffle to the last track");
    Check(WrapRoundBgmSelection(11, 10) == 0,
          "right wraps from the last track to shuffle");
    Check(WrapRoundBgmSelection(4, 0) == 0,
          "empty track list only exposes shuffle");
}

static void TestFadeAndMirrorRules(void) {
    Check(ClampRoundScreenFade(-1) == 0,
          "negative fade is fully transparent");
    Check(ClampRoundScreenFade(0x40) == 0x40,
          "fade preserves values in range");
    Check(ClampRoundScreenFade(0x80) == 0x7F &&
              ClampRoundScreenFade(1000) == 0x7F,
          "fade saturates at the PS1 brightness limit");

    Check(!IsRoundMirrorMode(PAD_START | PAD_R1),
          "partial mirror chord stays in normal mode");
    Check(IsRoundMirrorMode(PAD_START | PAD_R1 | PAD_L1),
          "complete mirror chord enables mirror mode");
    Check(IsRoundMirrorMode(PAD_START | PAD_R1 | PAD_L1 | PAD_CONFIRM),
          "unrelated held buttons do not cancel mirror mode");
}

static void TestBgmChoice(void) {
    const u8 order[4] = {3, 9, 1, 7};
    RoundBgmChoice choice;

    choice = ChooseRoundBgm(3, order, 4, 2);
    Check(choice.track == 2 && choice.shuffleIndex == 2,
          "manual selection preserves the shuffle cursor");

    choice = ChooseRoundBgm(10, order, 4, 2);
    Check(choice.track == 9 && choice.shuffleIndex == 2,
          "manual selection returns the logical tenth track");

    choice = ChooseRoundBgm(0, order, 4, 1);
    Check(choice.track == 9 && choice.shuffleIndex == 2,
          "shuffle consumes its selected logical track");

    choice = ChooseRoundBgm(0, order, 4, 3);
    Check(choice.track == 7 && choice.shuffleIndex == 0,
          "shuffle cursor wraps after its last entry");

    choice = ChooseRoundBgm(0, order, 0, 8);
    Check(choice.track == 0 && choice.shuffleIndex == 0,
          "empty shuffle is safe and deterministic");
}

int main(void) {
    TestRoundNumber();
    TestSelectionWrap();
    TestFadeAndMirrorRules();
    TestBgmChoice();

    if (s_failures != 0) {
        return 1;
    }
    puts("round screen progression and BGM rules are stable");
    return 0;
}
