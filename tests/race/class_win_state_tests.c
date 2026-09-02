#include <assert.h>

#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"

s32 g_BgmTrackCount;
s32 g_ClassWinCount;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];

static void SetClassWins(s32 winCount) {
    s32 index;

    for (index = 0; index < CLASS_RECORD_COUNT; index++) {
        g_ClassRecords[index].place = (s16)(index < winCount ? 1 : 2);
    }
}

static void TestRefreshesWinAndTrackCounts(void) {
    SetClassWins(4);
    g_ClassWinCount = -1;
    g_BgmTrackCount = -1;

    RefreshClassWinState();

    assert(g_ClassWinCount == 4);
    assert(g_BgmTrackCount == 9);

    SetClassWins(5);
    RefreshClassWinState();

    assert(g_ClassWinCount == 5);
    assert(g_BgmTrackCount == 10);
}

int main(void) {
    TestRefreshesWinAndTrackCounts();
    return 0;
}
