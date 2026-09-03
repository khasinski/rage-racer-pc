#include "game/car.h"
#include "game/fmv.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/state.h"

enum {
    COURSE_SELECT_SCENE = 6,
    CLASS_FMV_RETURN_SCENE = 7,
    ENDING_FMV_RETURN_SCENE = 0x21,
};

void AdvanceGrandPrixClass(void) {
    s32 maxClassReached;
    s32 nextClass;
    s32 *seriesMaxClass;

    if (!g_ClassCompleted) {
        g_SceneId = COURSE_SELECT_SCENE;
        return;
    }

    if (g_RaceProgress == NULL || g_CourseProgress == NULL ||
        (u32)g_SeriesSelection >= 2) {
        g_SceneId = COURSE_SELECT_SCENE;
        return;
    }

    if (g_SeriesCleared) {
        if (g_CarTable == NULL ||
            !IsFinalGrandPrixClass(g_SeriesSelection == 1,
                                   g_GrandPrixClass)) {
            g_SceneId = COURSE_SELECT_SCENE;
            return;
        }
        maxClassReached = g_RaceProgress->maxClassReached;
        ResetProgressSlot(g_CarTable, g_RaceProgress);
        g_RaceProgress->money = RACE_MAX_PRIZE_MONEY;
        g_RaceProgress->maxClassReached = maxClassReached;
        ResetCourseProgress(0);
        BeginEndingFmv(ENDING_FMV_RETURN_SCENE);
        return;
    }

    nextClass = NextGrandPrixClassForSeries(
        g_SeriesSelection, g_GrandPrixClass);
    if (nextClass < 0) {
        g_SceneId = COURSE_SELECT_SCENE;
        return;
    }

    BeginClassFmv(CLASS_FMV_RETURN_SCENE);
    g_GrandPrixClass = nextClass;
    g_RaceProgress->classIndex = nextClass;
    g_RaceProgress->course = 0;

    if (g_ClassPromoted) {
        g_RaceProgress->maxClassReached = nextClass;
        seriesMaxClass = &g_MaxClassReached[g_SeriesSelection];
        if (*seriesMaxClass < nextClass) {
            *seriesMaxClass = nextClass;
        }
    }

    ResetCourseProgress(nextClass);
}
