#include "common.h"
#include "game/save_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/player_car_internal.h"


void DrawSplitTimes(void) {
    s32 value;
    s32 tile;
    s32 timeout;
    s32 threshold;
    s32 finalValue;

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
    } else {
        goto split_current_done;
    }

    if (value <= threshold) {
        tile = 0x78CC;
    } else {
        tile = 0x7890;
    }
    DrawTimeValue(0x12, 0x2A, value, tile, 0x3E8);

split_current_done:
    timeout = 0x3E8;
    DrawTimeValue(0x12, 0x20, g_SplitTargetTime, 0x78CC, timeout);
    DrawSplitDelta(g_SplitSector, g_SplitSign);

    {
        s32 finalA0 = 0xFA;
        s32 finalA1 = 0x7C;
        s32 finalA3 = 0x78CC;

        finalValue = g_BestTotalTimes[g_RaceSeries][RageSeriesCourseIndex()][0];
        DrawTimeValue(finalA0, finalA1, finalValue, finalA3, timeout);
    }
}
