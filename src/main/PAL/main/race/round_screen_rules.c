#include "game/round_screen_internal.h"
#include "game/state.h"

#include <stddef.h>

s32 ClampRoundScreenFade(s32 value) {
    if (value < 0) {
        return 0;
    }
    return value < 0x80 ? value : 0x7F;
}

s32 IsRoundMirrorMode(u16 heldButtons) {
    const u16 mirrorChord = PAD_START | PAD_R1 | PAD_L1;

    return (heldButtons & mirrorChord) == mirrorChord;
}

s32 DetermineGrandPrixRound(const u8 bestPlaces[4], s32 classIndex,
                            s32 courseIndex) {
    s32 courseCount = classIndex < 2 ? 3 : 4;
    s32 round = 0;
    s32 course;

    if (bestPlaces == NULL || courseIndex < 0 || courseIndex >= 4) {
        return 0;
    }

    for (course = 0; course < courseCount; course++) {
        if (bestPlaces[course] != 0) {
            round++;
        }
    }
    if (bestPlaces[courseIndex] == 0) {
        round++;
    }
    return round;
}

s32 WrapRoundBgmSelection(s32 selection, s32 trackCount) {
    s32 optionCount = trackCount + 1;

    if (optionCount <= 0) {
        return 0;
    }
    selection %= optionCount;
    return selection < 0 ? selection + optionCount : selection;
}

RoundBgmChoice ChooseRoundBgm(s32 selection, const u8 *shuffleOrder,
                              s32 trackCount, s32 shuffleIndex) {
    RoundBgmChoice choice = {
        .track = 0,
        .shuffleIndex = 0,
    };

    if (selection == 0 && shuffleOrder != NULL && trackCount > 0) {
        shuffleIndex %= trackCount;
        if (shuffleIndex < 0) {
            shuffleIndex += trackCount;
        }
        choice.track = shuffleOrder[shuffleIndex];
        choice.shuffleIndex = (shuffleIndex + 1) % trackCount;
    } else if (selection > 0) {
        choice.track = selection - 1;
        choice.shuffleIndex = shuffleIndex;
    }

    return choice;
}
