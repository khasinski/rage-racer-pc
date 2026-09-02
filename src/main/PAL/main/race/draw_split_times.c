#include "game/save_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render.h"
#include "game/player_car_internal.h"

#include "rage/hud_config.h"

enum {
    TIME_DISPLAY_TIMEOUT = 1000,
};

void DrawSplitTimes(void) {
    s32 tile;

    if (!HudShowLapTimes()) {
        return;
    }

    if (SplitCurrentTimeVisible(g_SplitTimer, g_SectorIndex)) {
        if (SplitDeltaVisible(g_SplitTimer, g_SectorIndex, g_SplitSign,
                              g_LapCount, g_PlayerCar.lap)) {
            tile = SplitDeltaClut(g_SplitSign);
            DrawTimeValue(0x80, 0x50, g_SplitDelta, tile,
                          TIME_DISPLAY_TIMEOUT);
        }

        tile = SplitTimeClut(g_LastSectorTime);
        DrawTimeValue(HudLeftX(0x12), 0x2A, g_LastSectorTime, tile,
                      TIME_DISPLAY_TIMEOUT);
    }

    DrawTimeValue(HudLeftX(0x12), 0x20, g_SplitTargetTime,
                  0x78CC, TIME_DISPLAY_TIMEOUT);
    DrawSplitIndicator(g_SplitSector, g_SplitSign);

    DrawTimeValue(HudRightX(0xFA), 0x7C,
                  g_BestTotalTimes[g_RaceSeries][SeriesCourseIndex()][0],
                  0x78CC, TIME_DISPLAY_TIMEOUT);
}
