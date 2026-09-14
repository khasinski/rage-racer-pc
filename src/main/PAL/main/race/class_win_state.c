#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"

void RefreshClassWinState(void) {
    s32 wins = CountClassWins(g_ClassRecords, CLASS_RECORD_COUNT);

    g_BgmTrackCount = BgmTrackCountForClassWins(wins);
}
