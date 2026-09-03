#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"

#include <stdio.h>
#include <string.h>

s16 g_ExtraGrandPrixUnlocked;
s16 g_GrandPrixSeries;
s16 g_SeriesSelection;
s32 g_ClassClearFanfareTimer;
s32 g_ClassCompleted;
s32 g_ClassPromoted;
s32 g_ClassResultPlace;
s32 g_CourseIndex;
s32 g_GrandPrixClass;
s32 g_PlayerCarIndex;
s32 g_SeriesCleared;
CourseProgressState *g_CourseProgress;
GameRaceProgress *g_RaceProgress;
PlayerCarRuntime g_PlayerCar;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];

static CourseProgressState s_courseProgress;
static GameRaceProgress s_raceProgress;
static s32 s_carUnlockLevel;
static s32 s_bgmUpdates;
static s32 s_failures;

s32 GetCarUnlockLevel(s32 carIndex) {
    (void)carIndex;
    return s_carUnlockLevel;
}

void RefreshClassWinState(void) {
    s_bgmUpdates++;
}

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

static void Reset(void) {
    s32 record;

    memset(&s_courseProgress, 0, sizeof(s_courseProgress));
    memset(&s_raceProgress, 0, sizeof(s_raceProgress));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    for (record = 0; record < CLASS_RECORD_COUNT; record++) {
        g_ClassRecords[record].place = -1;
        g_ClassRecords[record].clears = 0;
    }
    g_ClassRecords[0].place = 0;
    g_CourseProgress = &s_courseProgress;
    g_RaceProgress = &s_raceProgress;
    g_CourseIndex = 0;
    g_GrandPrixClass = 0;
    g_GrandPrixSeries = 0;
    g_SeriesSelection = 0;
    g_PlayerCarIndex = 0;
    g_ExtraGrandPrixUnlocked = 0;
    g_ClassClearFanfareTimer = 99;
    g_ClassCompleted = 0;
    g_ClassPromoted = 0;
    g_ClassResultPlace = -1;
    g_SeriesCleared = 0;
    s_carUnlockLevel = 0;
    s_bgmUpdates = 0;
}

int main(void) {
    Reset();
    s_courseProgress.bestPlace[3] = 0xFF;
    g_PlayerCar.drive.racePosition = 2;
    CommitClassProgress();
    Check("course result is recorded", s_courseProgress.bestPlace[0], 2);
    Check("partial class stays incomplete", g_ClassCompleted, 0);
    Check("partial class has no grade", g_ClassResultPlace, 0);
    Check("partial class clears stale fanfare", g_ClassClearFanfareTimer, 0);
    Check("partial class does not update BGM", s_bgmUpdates, 0);

    Reset();
    s_courseProgress.bestPlace[0] = 1;
    s_courseProgress.bestPlace[1] = 1;
    s_courseProgress.bestPlace[3] = 0xFF;
    g_CourseIndex = 2;
    g_PlayerCar.drive.racePosition = 1;
    CommitClassProgress();
    Check("three-course class completes", g_ClassCompleted, 1);
    Check("first-place class grade", g_ClassResultPlace, 1);
    Check("class record stores grade", g_ClassRecords[0].place, 1);
    Check("first-place clear is counted", g_ClassRecords[0].clears, 1);
    Check("next class record unlocks", g_ClassRecords[1].place, 0);
    Check("class clear starts fanfare", g_ClassClearFanfareTimer,
          CLASS_CLEAR_FANFARE_DURATION_FRAMES);
    Check("completed class updates BGM", s_bgmUpdates, 1);
    Check("new class is promoted", g_ClassPromoted, 1);

    Reset();
    s_courseProgress.bestPlace[0] = 1;
    s_courseProgress.bestPlace[1] = 1;
    s_courseProgress.bestPlace[3] = 0xFF;
    g_CourseIndex = 2;
    g_PlayerCar.drive.racePosition = 1;
    s_carUnlockLevel = 2;
    CommitClassProgress();
    Check("over-level car marks unlock pending",
          s_courseProgress.unlockPending, 1);
    Check("unlock pending blocks grade", g_ClassResultPlace, 0);
    Check("blocked grade has no fanfare", g_ClassClearFanfareTimer, 0);
    Check("blocked grade does not count clear", g_ClassRecords[0].clears, 0);

    Reset();
    memset(s_courseProgress.bestPlace, 1,
           sizeof(s_courseProgress.bestPlace));
    g_GrandPrixClass = 4;
    g_CourseIndex = 3;
    g_PlayerCar.drive.racePosition = 1;
    g_ClassRecords[4].place = 0;
    s_raceProgress.maxClassReached = 4;
    CommitClassProgress();
    Check("standard finale clears series", g_SeriesCleared, 1);
    Check("standard finale unlocks Extra GP", g_ExtraGrandPrixUnlocked, 1);
    Check("series clear is not a promotion", g_ClassPromoted, 0);
    Check("standard finale unlocks Extra record", g_ClassRecords[6].place, 0);

    Reset();
    g_CourseProgress = NULL;
    CommitClassProgress();
    Check("missing course progress stays incomplete", g_ClassCompleted, 0);
    Check("missing course progress clears stale grade", g_ClassResultPlace, 0);
    Check("missing course progress clears stale fanfare",
          g_ClassClearFanfareTimer, 0);

    Reset();
    g_RaceProgress = NULL;
    CommitClassProgress();
    Check("missing race progress stays incomplete", g_ClassCompleted, 0);

    Reset();
    g_GrandPrixSeries = 1;
    g_GrandPrixClass = GRAND_PRIX_FINAL_CLASS_INDEX;
    CommitClassProgress();
    Check("invalid series-class pair stays incomplete", g_ClassCompleted, 0);
    Check("invalid series-class pair does not update BGM", s_bgmUpdates, 0);

    return s_failures != 0;
}
