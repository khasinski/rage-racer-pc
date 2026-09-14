#include <assert.h>

#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"

s32 g_BgmTrackCount;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];

static void SetClassWins(s32 winCount) {
    s32 index;

    for (index = 0; index < CLASS_RECORD_COUNT; index++) {
        g_ClassRecords[index].place = (s16)(index < winCount ? 1 : 2);
    }
}

static void TestRefreshesTrackCount(void) {
    SetClassWins(4);
    g_BgmTrackCount = -1;

    RefreshClassWinState();

    assert(g_BgmTrackCount == 9);

    SetClassWins(5);
    RefreshClassWinState();

    assert(g_BgmTrackCount == 10);
}

int main(void) {
    TestRefreshesTrackCount();
    return 0;
}
