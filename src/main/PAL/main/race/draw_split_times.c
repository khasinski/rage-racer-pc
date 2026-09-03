#include "game/save_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/player_car_internal.h"

#include "rage/hud_config.h"

enum {
    MILLISECONDS_PER_SECOND = 1000,
    SPLIT_DELTA_X = 0x80,
    SPLIT_DELTA_Y = 0x50,
    SPLIT_TIME_X = 0x12,
    LAST_SPLIT_TIME_Y = 0x2A,
    TARGET_SPLIT_TIME_Y = 0x20,
    BEST_TOTAL_TIME_X = 0xFA,
    BEST_TOTAL_TIME_Y = 0x7C,
    DEFAULT_TIME_CLUT = 0x78CC,
};

void DrawSplitTimes(void) {
    s32 clut;

    if (!HudShowLapTimes()) {
        return;
    }

    if (SplitCurrentTimeVisible(g_SplitTimer, g_SectorIndex)) {
        if (SplitDeltaVisible(g_SplitTimer, g_SectorIndex, g_SplitSign,
                              g_LapCount, g_PlayerCar.lap)) {
            clut = SplitDeltaClut(g_SplitSign);
            DrawTimeValue(SPLIT_DELTA_X, SPLIT_DELTA_Y, g_SplitDelta, clut,
                          MILLISECONDS_PER_SECOND);
        }

        clut = SplitTimeClut(g_LastSectorTime);
        DrawTimeValue(HudLeftX(SPLIT_TIME_X), LAST_SPLIT_TIME_Y,
                      g_LastSectorTime, clut, MILLISECONDS_PER_SECOND);
    }

    DrawTimeValue(HudLeftX(SPLIT_TIME_X), TARGET_SPLIT_TIME_Y,
                  g_SplitTargetTime, DEFAULT_TIME_CLUT,
                  MILLISECONDS_PER_SECOND);
    DrawSplitIndicator(SplitDisplaySectorIndex(g_SplitSector), g_SplitSign);

    DrawTimeValue(HudRightX(BEST_TOTAL_TIME_X), BEST_TOTAL_TIME_Y,
                  g_BestTotalTimes[RaceSeriesIndex(g_RaceSeries)]
                                  [SeriesCourseIndex()][0],
                  DEFAULT_TIME_CLUT, MILLISECONDS_PER_SECOND);
}
