#include "game/round_screen_internal.h"
#include "game/state.h"

#include <stddef.h>
#include <stdint.h>

enum {
    ROUND_SCREEN_SERIES_COUNT = 2,
    ROUND_SCREEN_CLASS_COUNT = 6,
    ROUND_SCREEN_BGM_TRACK_COUNT = 10,
    ROUND_SCREEN_TIMER_LIMIT = 10000,
};

s32 ClampRoundScreenFade(s32 value) {
    if (value < 0) {
        return 0;
    }
    return value < 0x80 ? value : 0x7F;
}

s32 RoundScreenFadeFromTimer(s32 timer, s32 delay) {
    int64_t value = (int64_t)timer * 4 - delay;

    if (value <= 0) {
        return 0;
    }
    return value < 0x80 ? (s32)value : 0x7F;
}

s32 NextRoundScreenTimer(s32 timer) {
    if (timer < 0) {
        return 0;
    }
    return timer < ROUND_SCREEN_TIMER_LIMIT ? timer + 1
                                            : ROUND_SCREEN_TIMER_LIMIT;
}

s32 IsRoundMirrorMode(u16 heldButtons) {
    const u16 mirrorChord = PAD_START | PAD_R1 | PAD_L1;

    return (heldButtons & mirrorChord) == mirrorChord;
}

s32 IsRoundScreenAssetLoadComplete(s32 loadState, s32 loadFailed) {
    return loadState == 0 && loadFailed == 0;
}

s32 DetermineGrandPrixRound(const u8 bestPlaces[4], s32 classIndex,
                            s32 courseIndex) {
    s32 courseCount = classIndex < 2 ? 3 : 4;
    s32 round = 0;
    s32 course;

    if (bestPlaces == NULL || (u32)classIndex >= ROUND_SCREEN_CLASS_COUNT ||
        courseIndex < 0 || courseIndex >= 4) {
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

s32 RoundScreenTableIndicesValid(s32 series, s32 classIndex,
                                 s32 grandPrixMode) {
    if ((u32)series >= ROUND_SCREEN_SERIES_COUNT) {
        return 0;
    }
    return grandPrixMode == 0 || (u32)classIndex < ROUND_SCREEN_CLASS_COUNT;
}

s32 ClampRoundBgmTrackCount(s32 trackCount) {
    if (trackCount < 0) {
        return 0;
    }
    return trackCount < ROUND_SCREEN_BGM_TRACK_COUNT
               ? trackCount
               : ROUND_SCREEN_BGM_TRACK_COUNT;
}

s32 WrapRoundBgmSelection(s32 selection, s32 trackCount) {
    s32 optionCount = ClampRoundBgmTrackCount(trackCount) + 1;

    selection %= optionCount;
    return selection < 0 ? selection + optionCount : selection;
}

RoundBgmChoice ChooseRoundBgm(s32 selection, const u8 *shuffleOrder,
                              s32 trackCount, s32 shuffleIndex) {
    RoundBgmChoice choice = {
        .track = 0,
        .shuffleIndex = 0,
    };

    trackCount = ClampRoundBgmTrackCount(trackCount);
    selection = WrapRoundBgmSelection(selection, trackCount);

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
