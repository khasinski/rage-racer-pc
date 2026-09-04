#include <assert.h>
#include <limits.h>

#include "game/race_hud_internal.h"
#include "game/race_internal.h"

int main(void) {
    assert(SplitCurrentTimeVisible(SPLIT_DISPLAY_FRAMES - 1, 0));
    assert(!SplitCurrentTimeVisible(SPLIT_DISPLAY_FRAMES, 0));
    assert(!SplitCurrentTimeVisible(0, -1));
    assert(!SplitCurrentTimeVisible(-1, 0));
    assert(!SplitCurrentTimeVisible(0, SPLIT_SECTOR_COUNT));

    assert(SplitDeltaVisible(SPLIT_DISPLAY_FRAMES - 1, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(SPLIT_DISPLAY_FRAMES, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(SPLIT_DISPLAY_FRAMES - 1, 0, 0, 3, 3));
    assert(!SplitDeltaVisible(SPLIT_DISPLAY_FRAMES - 1, 0, 1, 2, 3));

    assert(SplitDeltaClut(1) == 0x7810);
    assert(SplitDeltaClut(-1) == 0x780F);
    assert(SplitTimeClut(SPLIT_TIME_MAX_MS) == 0x78CC);
    assert(SplitTimeClut(SPLIT_TIME_MAX_MS + 1) == 0x7890);
    assert(SplitTimeClut(-1) == 0x7890);
    assert(!SplitTargetVisible(-2, 100000));
    assert(!SplitTargetVisible(0, 0));
    assert(!SplitTargetVisible(0, -1));
    assert(SplitTargetVisible(0, 100000));
    assert(RaceSeriesIndex(0) == 0);
    assert(RaceSeriesIndex(1) == 1);
    assert(RaceSeriesIndex(INT_MIN) == 0);
    assert(RaceSeriesIndex(INT_MAX) == 0);
    assert(SplitDisplaySectorIndex(2) == 2);
    assert(SplitDisplaySectorIndex(-1) == 0);
    assert(SplitDisplaySectorIndex(INT_MAX) == 0);
    return 0;
}
