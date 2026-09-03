#include <assert.h>
#include <limits.h>

#include "game/race_hud_internal.h"

int main(void) {
    assert(SplitCurrentTimeVisible(59, 0));
    assert(!SplitCurrentTimeVisible(60, 0));
    assert(!SplitCurrentTimeVisible(0, -1));
    assert(!SplitCurrentTimeVisible(-1, 0));
    assert(!SplitCurrentTimeVisible(0, 3));

    assert(SplitDeltaVisible(59, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(60, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(59, 0, 0, 3, 3));
    assert(!SplitDeltaVisible(59, 0, 1, 2, 3));

    assert(SplitDeltaClut(1) == 0x7810);
    assert(SplitDeltaClut(-1) == 0x780F);
    assert(SplitTimeClut(599998) == 0x78CC);
    assert(SplitTimeClut(599999) == 0x7890);
    assert(SplitTimeClut(-1) == 0x7890);
    assert(SplitRecordSeriesIndex(0) == 0);
    assert(SplitRecordSeriesIndex(1) == 1);
    assert(SplitRecordSeriesIndex(INT_MIN) == 0);
    assert(SplitRecordSeriesIndex(INT_MAX) == 0);
    assert(SplitDisplaySectorIndex(2) == 2);
    assert(SplitDisplaySectorIndex(-1) == 0);
    assert(SplitDisplaySectorIndex(INT_MAX) == 0);
    return 0;
}
