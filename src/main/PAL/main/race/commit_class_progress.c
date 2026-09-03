#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"

void CommitClassProgress(void) {
    s32 classRecordIndex;
    s32 nextRecordIndex;
    u8 *coursePlace;
    s32 courseCount;
    s32 grade;
    s32 carUnlockLevel;

    g_ClassClearFanfareTimer = 0;
    g_ClassCompleted = 0;
    g_ClassPromoted = 0;
    g_ClassResultPlace = 0;
    g_SeriesCleared = 0;

    classRecordIndex = GrandPrixClassRecordIndex(
        g_GrandPrixSeries, g_GrandPrixClass);
    if (g_CourseProgress == NULL || g_RaceProgress == NULL ||
        classRecordIndex < 0) {
        return;
    }

    coursePlace = &g_CourseProgress->bestPlace[SeriesCourseIndex()];
    *coursePlace = (u8)BestRacePlace(*coursePlace,
                                    g_PlayerCar.drive.racePosition);

    carUnlockLevel = GetCarUnlockLevel(g_PlayerCarIndex);
    if (g_GrandPrixClass < carUnlockLevel) {
        g_CourseProgress->unlockPending = 1;
    }

    courseCount = GrandPrixCourseCount(g_GrandPrixClass);
    g_ClassCompleted = GrandPrixClassIsComplete(
        g_CourseProgress->bestPlace, courseCount);
    if (g_ClassCompleted) {
        nextRecordIndex = NextUnlockedClassRecord(classRecordIndex);
        if (nextRecordIndex >= 0 && g_ClassRecords[nextRecordIndex].place == -1) {
            g_ClassRecords[nextRecordIndex].place = 0;
        }

        grade = ComputeClassGradeForPlaces(g_CourseProgress->bestPlace,
                                           g_CourseProgress->unlockPending);
        g_ClassResultPlace = grade;
        if (grade != 0) {
            g_ClassRecords[classRecordIndex].place = (s16)BestClassGrade(
                g_ClassRecords[classRecordIndex].place, grade);
            g_ClassClearFanfareTimer = CLASS_CLEAR_FANFARE_DURATION_FRAMES;
        }

        RefreshClassWinState();
        g_ClassRecords[classRecordIndex].clears = UpdatedClassClearCount(
            g_ClassRecords[classRecordIndex].clears, g_ClassResultPlace);
    } else {
        g_ClassResultPlace = 0;
    }

    g_SeriesCleared = g_ClassCompleted &&
        IsFinalGrandPrixClass(g_SeriesSelection == 1, g_GrandPrixClass);
    if (g_SeriesCleared) {
        g_ExtraGrandPrixUnlocked = 1;
    }

    g_ClassPromoted = g_ClassCompleted && !g_SeriesCleared &&
        g_RaceProgress->maxClassReached < g_GrandPrixClass + 1;
}
