#include "game/race.h"
#include "game/race_internal.h"

enum {
    RESULT_PLACE_FIRST = 1,
    RESULT_PLACE_LAST = 3,
    STANDARD_GRAND_PRIX_CLASS_COUNT = 6,
    EXTRA_GRAND_PRIX_CLASS_COUNT = 5,
    EXTRA_GRAND_PRIX_NAME_OFFSET = STANDARD_GRAND_PRIX_CLASS_COUNT,
    INVALID_GRAND_PRIX_NAME_INDEX = -1,
};

s32 ResultCourseNameY(s32 grandPrixMode) {
    return grandPrixMode != 0 ? 0x3C : 0x39;
}

s32 IsValidRaceResultPlace(s32 racePosition) {
    return racePosition >= RESULT_PLACE_FIRST &&
        racePosition <= RESULT_PLACE_LAST;
}

s32 ShouldDrawClassPlaceBanner(s32 classPlace, s32 prizeScreenState) {
    return IsValidRaceResultPlace(classPlace) &&
        prizeScreenState >= PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM;
}

s32 GrandPrixNameIndex(s32 extraSeries, s32 classIndex) {
    s32 classCount = extraSeries != 0
        ? EXTRA_GRAND_PRIX_CLASS_COUNT
        : STANDARD_GRAND_PRIX_CLASS_COUNT;

    if (classIndex < 0 || classIndex >= classCount) {
        return INVALID_GRAND_PRIX_NAME_INDEX;
    }
    return classIndex +
        (extraSeries != 0 ? EXTRA_GRAND_PRIX_NAME_OFFSET : 0);
}
