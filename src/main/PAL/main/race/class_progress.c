#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/state.h"

void UpdateBgmTrackCount(void) {
    g_ClassWinCount = CountClassWins(g_ClassRecords, CLASS_RECORD_COUNT);
    g_BgmTrackCount = BgmTrackCountForClassWins(g_ClassWinCount);
}

void CommitClassProgress(void) {
    s32 classRecordIndex;
    s32 nextRecordIndex;
    u8 *coursePlace;
    s32 courseCount;
    s32 grade;
    s32 carUnlockLevel;

    coursePlace = &g_CourseProgress->bestPlace[SeriesCourseIndex()];
    g_ClassClearFanfareTimer = 0;

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
        classRecordIndex = g_GrandPrixSeries * 6 + g_GrandPrixClass;
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

        UpdateBgmTrackCount();
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

void EnterPrizeScreen(void) {
    s32 courseIndex;
    s32 classIndex;
    const s32 *prizes;

    g_SceneTimer = 0x100;
    g_FrameSyncThreshold = 0x80;
    g_RaceProgress->money.value = ClampPrizeMoney(g_RaceProgress->money.value);

    courseIndex = SeriesCourseIndex();
    classIndex = g_GrandPrixClass;
    prizes = g_PrizeMoney.values[courseIndex][classIndex];
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = PrizeForRacePosition(
        prizes, PRIZE_PLACE_COUNT, g_PlayerCar.drive.racePosition);
    g_SceneId = 0x13;

    g_PromotionBonus = PromotionBonusForClass(
        g_PromotionBonusTable, PROMOTION_BONUS_COUNT, classIndex,
        g_ClassPromoted);

    g_PrizeCountStep = PrizeCountStep(prizes[PRIZE_PLACE_THIRD], 80);
    g_BonusCountStep = PrizeCountStep(g_PromotionBonus, 250);
}
