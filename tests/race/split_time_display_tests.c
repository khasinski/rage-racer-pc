#include <assert.h>

#include "game/race_hud_internal.h"

int main(void) {
    assert(SplitCurrentTimeVisible(59, 0));
    assert(!SplitCurrentTimeVisible(60, 0));
    assert(!SplitCurrentTimeVisible(0, -1));

    assert(SplitDeltaVisible(59, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(60, 0, 1, 3, 3));
    assert(!SplitDeltaVisible(59, 0, 0, 3, 3));
    assert(!SplitDeltaVisible(59, 0, 1, 2, 3));

    assert(SplitDeltaClut(1) == 0x7810);
    assert(SplitDeltaClut(-1) == 0x780F);
    assert(SplitTimeClut(599998) == 0x78CC);
    assert(SplitTimeClut(599999) == 0x7890);
    return 0;
}
