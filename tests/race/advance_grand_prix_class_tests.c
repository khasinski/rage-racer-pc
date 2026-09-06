#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/fmv.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"
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
CourseProgressState *g_CourseProgress;
CarEntry *g_CarTable;
GameCdLoadEntry g_StreamCdEntries[FMV_STREAM_COUNT];
GameCdLoadEntry *g_StreamLoc;
u32 g_StreamFrameCount;

static GameRaceProgress s_progress;
static CourseProgressState s_courseProgress;
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

void BeginFmv(s32 returnScene) {
    if (returnScene == 7) s_classFmvReturnScene = returnScene;
    else s_endingFmvReturnScene = returnScene;
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
    g_CourseProgress = &s_courseProgress;
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
    g_StreamLoc = NULL;
    g_StreamFrameCount = 0;
    for (s32 i = 0; i < FMV_STREAM_COUNT; ++i)
        g_StreamCdEntries[i].size = (u32)(100 + i);
}

int main(void) {
    static const s32 classInputs[] = {-1, 0, 1, 2, 3, 4, 5, 6};
    for (s32 series = 0; series < 2; ++series) {
        for (u32 i = 0; i < sizeof(classInputs) / sizeof(classInputs[0]); ++i) {
            s32 index = classInputs[i];
            s32 clamped = index < 0 ? 0 : index > 3 ? 3 : index;
            s32 stream = 1 + series * 4 + clamped;
            Reset();
            g_SeriesSelection = (s16)series;
            g_GrandPrixClass = index;
            BeginClassFmv(7);
            Check("direct class movie preserves retail clamping",
                  g_StreamLoc == &g_StreamCdEntries[stream], 1);
            Check("direct class movie frame count", g_StreamFrameCount, 100 + stream);
        }
    }
    for (s32 series = 0; series < 2; ++series) {
        for (s32 completed = 0; completed < (series ? 5 : 4); ++completed) {
            Reset();
            g_ClassCompleted = 1;
            g_SeriesSelection = (s16)series;
            g_GrandPrixClass = completed;
            AdvanceGrandPrixClass();
            s32 stream = (series ? 5 : 1) + (completed < 4 ? completed : 3);
            Check("completed class selects its own movie",
                  g_StreamLoc == &g_StreamCdEntries[stream], 1);
            Check("selected stream frame count", (s32)g_StreamFrameCount, 100 + stream);
            Check("promotion increments after stream selection", g_GrandPrixClass, completed + 1);
            Check("promotion returns through class handler", s_classFmvReturnScene, 7);
        }
        Reset();
        g_ClassCompleted = g_SeriesCleared = 1;
        g_SeriesSelection = (s16)series;
        g_GrandPrixClass = series ? 5 : 4;
        AdvanceGrandPrixClass();
        Check("both series select ending stream", g_StreamLoc == &g_StreamCdEntries[10], 1);
        Check("ending frame count", (s32)g_StreamFrameCount, 110);
    }
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
    g_GrandPrixClass = 4;
    s_progress.maxClassReached = 5;
    s_progress.money = 123;
    AdvanceGrandPrixClass();
    Check("series clear resets slot", s_resetProgressCalls, 1);
    Check("series clear preserves unlock level", s_progress.maxClassReached, 5);
    Check("series clear awards maximum money", s_progress.money,
          RACE_MAX_PRIZE_MONEY);
    Check("series clear resets beginner progress", s_resetCourseMode, 0);
    Check("ending FMV return scene", s_endingFmvReturnScene, 0x21);
    Check("series clear skips class FMV", s_classFmvReturnScene, -1);

    Reset();
    g_ClassCompleted = 1;
    g_RaceProgress = NULL;
    AdvanceGrandPrixClass();
    Check("missing save returns to course select", g_SceneId, 6);
    Check("missing save does not start class FMV", s_classFmvReturnScene, -1);

    Reset();
    g_ClassCompleted = 1;
    g_SeriesSelection = 2;
    AdvanceGrandPrixClass();
    Check("invalid series returns to course select", g_SceneId, 6);

    Reset();
    g_ClassCompleted = 1;
    g_SeriesCleared = 1;
    g_SeriesSelection = 2;
    g_GrandPrixClass = 4;
    AdvanceGrandPrixClass();
    Check("invalid cleared series returns to course select", g_SceneId, 6);
    Check("invalid cleared series does not reset save", s_resetProgressCalls,
          0);

    Reset();
    g_ClassCompleted = 1;
    g_GrandPrixClass = 4;
    AdvanceGrandPrixClass();
    Check("standard final cannot advance without clear", g_SceneId, 6);

    Reset();
    g_ClassCompleted = 1;
    g_SeriesCleared = 1;
    g_GrandPrixClass = 2;
    AdvanceGrandPrixClass();
    Check("non-final class cannot clear series", g_SceneId, 6);
    Check("invalid series clear does not reset save", s_resetProgressCalls, 0);

    return s_failures != 0;
}
