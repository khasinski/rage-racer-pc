#include "game/save_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/player_car_internal.h"

#include "rage/hud_config.h"


void DrawSplitTimes(void) {
    s32 value;
    s32 tile;
    s32 timeout;
    s32 threshold;

    if (!HudShowLapTimes()) return;

    if (g_SplitTimer >= 0x3C) {
        threshold = 0x927BE;
        value = g_LapTimeMs;

    } else if (g_SectorIndex >= 0) {
        if (g_SplitSign != 0) {
            if (g_LapCount >= g_PlayerCar.lap) {
                value = g_SplitDelta;
                if (g_SplitSign > 0) {
                    tile = 0x7810;
                } else {
                    tile = 0x780F;
                }
                DrawTimeValue(0x80, 0x50, value, tile, 0x3E8);
            }
        }
        threshold = 0x927BE;
        value = g_LastSectorTime;

        tile = value <= threshold ? 0x78CC : 0x7890;
        DrawTimeValue(HudLeftX(0x12), 0x2A, value, tile, 0x3E8);
    }

    timeout = 0x3E8;
    DrawTimeValue(HudLeftX(0x12), 0x20, g_SplitTargetTime,
                  0x78CC, timeout);
    DrawSplitDelta(g_SplitSector, g_SplitSign);

    DrawTimeValue(HudRightX(0xFA), 0x7C,
                  g_BestTotalTimes[g_RaceSeries][SeriesCourseIndex()][0],
                  0x78CC, timeout);
}
