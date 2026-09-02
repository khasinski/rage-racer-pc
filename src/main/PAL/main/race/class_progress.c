#include <stdio.h>

#include "game/audio.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/sound.h"
#include "game/state.h"

enum { MAX_PRIZE_MONEY = 999999999 };

void UpdateBgmTrackCount(void) {
    enum {
        CLASS_RECORD_COUNT = 11,
        DEFAULT_BGM_TRACK_COUNT = 9,
        UNLOCKED_BGM_TRACK_COUNT = 10,
        BGM_UNLOCK_WIN_COUNT = 5,
    };

    g_ClassWinCount = CountClassWins(g_ClassRecords, CLASS_RECORD_COUNT);
    g_BgmTrackCount = g_ClassWinCount < BGM_UNLOCK_WIN_COUNT
        ? DEFAULT_BGM_TRACK_COUNT
        : UNLOCKED_BGM_TRACK_COUNT;
}

void DrawPrizeMoneyPanel(s32 yOffset) {
    char moneyText[16];

    if (g_RaceProgress->money.value > MAX_PRIZE_MONEY) {
        g_RaceProgress->money.value = MAX_PRIZE_MONEY;
    }
    DrawProportionalText(0x10, yOffset + 128, g_CaptionPrizeMoney, 0x7812);
    snprintf(moneyText, sizeof(moneyText), g_FmtMoney, g_PrizeAmount);
    DrawProportionalText(0x12, yOffset + 140, moneyText, 0x7812);
    DrawProportionalText(0x10, yOffset + 160, g_CaptionTotalMoney, 0x7812);
    snprintf(moneyText, sizeof(moneyText), g_FmtMoney,
             g_RaceProgress->money.value);
    DrawProportionalText(0x12, yOffset + 172, moneyText, 0x7812);
    if (g_ClassPromoted) {
        DrawProportionalText(0x10, yOffset + 192, g_CaptionPromotionBonus,
                             0x7812);
        snprintf(moneyText, sizeof(moneyText), g_FmtMoney, g_PromotionBonus);
        DrawProportionalText(0x12, yOffset + 204, moneyText, 0x7812);
    }
}

void CommitClassProgress(void) {
    s32 classRecordIndex;
    s32 nextRecordIndex;
    u8 *coursePlace;
    s32 courseCount;
    s32 completedCourses;
    s32 i;
    s32 grade;
    s32 carUnlockLevel;

    coursePlace = &g_CourseProgress->bestPlace[SeriesCourseIndex()];
    g_ClassClearFanfareTimer = 0;

    if (*coursePlace == 0 || g_PlayerCar.drive.racePosition < *coursePlace) {
        *coursePlace = g_PlayerCar.drive.racePosition;
    }

    carUnlockLevel = GetCarUnlockLevel(g_PlayerCarIndex);
    if (g_GrandPrixClass < carUnlockLevel) {
        g_CourseProgress->unlockPending = 1;
    }

    courseCount = GrandPrixCourseCount(g_GrandPrixClass);
    completedCourses = 0;
    for (i = 0; i < courseCount; i++) {
        if (g_CourseProgress->bestPlace[i] != 0) {
            completedCourses++;
        }
    }

    g_ClassCompleted = completedCourses == courseCount;
    if (g_ClassCompleted) {
        classRecordIndex = g_GrandPrixSeries * 6 + g_GrandPrixClass;
        nextRecordIndex = NextUnlockedClassRecord(classRecordIndex);
        if (nextRecordIndex >= 0 && g_ClassRecords[nextRecordIndex].place == -1) {
            g_ClassRecords[nextRecordIndex].place = 0;
        }

        grade = ComputeClassGrade();
        g_ClassResultPlace = grade;
        if (grade != 0) {
            if (g_ClassRecords[classRecordIndex].place == 0 ||
                grade < g_ClassRecords[classRecordIndex].place) {
                g_ClassRecords[classRecordIndex].place = (u16)grade;
            }
            g_ClassClearFanfareTimer = 0xD2;
        }

        UpdateBgmTrackCount();
        if (g_ClassResultPlace == 1 &&
            (s16)g_ClassRecords[classRecordIndex].clears < 99) {
            g_ClassRecords[classRecordIndex].clears++;
        }
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

void AdvanceGrandPrixClass(void) {
    s32 maxClassReached;
    s32 nextClass;
    s32 *seriesMaxClass;

    if (!g_ClassCompleted) {
        g_SceneId = 6;
        return;
    }

    if (g_SeriesCleared) {
        maxClassReached = g_RaceProgress->maxClassReached;
        ResetProgressSlot(g_CarTable, g_RaceProgress);
        g_RaceProgress->money.value = MAX_PRIZE_MONEY;
        g_RaceProgress->maxClassReached = maxClassReached;
        ResetCourseProgress(0);
        BeginEndingFmv(0x21);
        return;
    }

    BeginClassFmv(7);
    nextClass = g_GrandPrixClass + 1;
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

void EnterPrizeScreen(void) {
    s32 courseIndex;
    s32 classIndex;

    g_SceneTimer = 0x100;
    g_FrameSyncThreshold = 0x80;

    courseIndex = g_CourseIndex;
    classIndex = g_GrandPrixClass;
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = g_PrizeMoney.values[courseIndex][classIndex]
                                      [g_PlayerCar.drive.racePosition - 1];
    g_SceneId = 0x13;

    if (g_ClassPromoted) {
        g_PromotionBonus = g_PromotionBonusTable[classIndex];
    } else {
        g_PromotionBonus = 0;
    }

    g_PrizeCountStep = PrizeCountStep(
        g_PrizeMoney.values[courseIndex][classIndex][PRIZE_PLACE_THIRD], 80);
    g_BonusCountStep = PrizeCountStep(g_PromotionBonusTable[classIndex], 250);
}

void TickClassClearFanfare(void) {
    if (g_ClassClearFanfareTimer > 0) {
        g_ClassClearFanfareTimer--;
    }
    if (g_ClassClearFanfareTimer == 0xB4) {
        PlaySoundCue(0x42);
    }
}
