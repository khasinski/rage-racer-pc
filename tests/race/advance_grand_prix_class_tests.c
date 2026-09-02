#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_ClassCompleted;
s32 g_ClassPromoted;
s32 g_GrandPrixClass;
s32 g_MaxClassReached[2];
s32 g_SceneId;
s32 g_SeriesCleared;
s16 g_SeriesSelection;
GameRaceProgress *g_RaceProgress;
CarEntry *g_CarTable;

static GameRaceProgress s_progress;
static CarEntry s_cars[GAME_CAR_COUNT];
static s32 s_resetProgressCalls;
static s32 s_resetCourseMode;
static s32 s_classFmvReturnScene;
static s32 s_endingFmvReturnScene;
static s32 s_failures;

void ResetProgressSlot(CarEntry *cars, GameRaceProgress *progress) {
    (void)cars;
    s_resetProgressCalls++;
    memset(progress, 0, sizeof(*progress));
    progress->maxClassReached = -1;
}

void ResetCourseProgress(s32 mode) {
    s_resetCourseMode = mode;
}

void BeginClassFmv(s32 returnScene) {
    s_classFmvReturnScene = returnScene;
}

void BeginEndingFmv(s32 returnScene) {
    s_endingFmvReturnScene = returnScene;
}

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

static void Reset(void) {
    memset(&s_progress, 0, sizeof(s_progress));
    memset(g_MaxClassReached, 0, sizeof(g_MaxClassReached));
    g_RaceProgress = &s_progress;
    g_CarTable = s_cars;
    g_ClassCompleted = 0;
    g_ClassPromoted = 0;
    g_GrandPrixClass = 2;
    g_SeriesCleared = 0;
    g_SeriesSelection = 0;
    g_SceneId = -1;
    s_resetProgressCalls = 0;
    s_resetCourseMode = -1;
    s_classFmvReturnScene = -1;
    s_endingFmvReturnScene = -1;
}

int main(void) {
    Reset();
    AdvanceGrandPrixClass();
    Check("unfinished class returns to course select", g_SceneId, 6);
    Check("unfinished class does not reset progress", s_resetProgressCalls, 0);

    Reset();
    g_ClassCompleted = 1;
    s_progress.course = 3;
    s_progress.maxClassReached = 2;
    AdvanceGrandPrixClass();
    Check("class FMV return scene", s_classFmvReturnScene, 7);
    Check("next live class", g_GrandPrixClass, 3);
    Check("next saved class", s_progress.classIndex, 3);
    Check("next class starts at first course", s_progress.course, 0);
    Check("existing unlock level is retained", s_progress.maxClassReached, 2);
    Check("next course progress is reset", s_resetCourseMode, 3);

    Reset();
    g_ClassCompleted = 1;
    g_ClassPromoted = 1;
    g_SeriesSelection = 1;
    g_MaxClassReached[1] = 1;
    AdvanceGrandPrixClass();
    Check("promotion advances slot unlock", s_progress.maxClassReached, 3);
    Check("promotion advances series unlock", g_MaxClassReached[1], 3);

    Reset();
    g_ClassCompleted = 1;
    g_ClassPromoted = 1;
    g_SeriesSelection = 1;
    g_MaxClassReached[1] = 4;
    AdvanceGrandPrixClass();
    Check("promotion does not lower series unlock", g_MaxClassReached[1], 4);

    Reset();
    g_ClassCompleted = 1;
    g_SeriesCleared = 1;
    s_progress.maxClassReached = 5;
    s_progress.money.value = 123;
    AdvanceGrandPrixClass();
    Check("series clear resets slot", s_resetProgressCalls, 1);
    Check("series clear preserves unlock level", s_progress.maxClassReached, 5);
    Check("series clear awards maximum money", s_progress.money.value,
          RACE_MAX_PRIZE_MONEY);
    Check("series clear resets beginner progress", s_resetCourseMode, 0);
    Check("ending FMV return scene", s_endingFmvReturnScene, 0x21);
    Check("series clear skips class FMV", s_classFmvReturnScene, -1);

    return s_failures != 0;
}
