#include "game/save_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/player_car_internal.h"

#include "rage/hud_config.h"

enum {
    SPLIT_DISPLAY_FRAMES = 60,
    MAX_DISPLAY_TIME_MS = 599998,
    TIME_DISPLAY_TIMEOUT = 1000,
};

void DrawSplitTimes(void) {
    s32 tile;

    if (!HudShowLapTimes()) {
        return;
    }

    if (g_SplitTimer < SPLIT_DISPLAY_FRAMES && g_SectorIndex >= 0) {
        if (g_SplitSign != 0 && g_LapCount >= g_PlayerCar.lap) {
            tile = g_SplitSign > 0 ? 0x7810 : 0x780F;
            DrawTimeValue(0x80, 0x50, g_SplitDelta, tile,
                          TIME_DISPLAY_TIMEOUT);
        }

        tile = g_LastSectorTime <= MAX_DISPLAY_TIME_MS ? 0x78CC : 0x7890;
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
