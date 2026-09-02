#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"

void RefreshClassWinState(void) {
    g_ClassWinCount = CountClassWins(g_ClassRecords, CLASS_RECORD_COUNT);
    g_BgmTrackCount = BgmTrackCountForClassWins(g_ClassWinCount);
}
