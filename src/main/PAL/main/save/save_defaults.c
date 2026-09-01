#include "game/audio.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"

#include <string.h>

void ResetProgressSlot(CarEntry *slot, GameRaceProgress *progress) {
    memcpy(slot, g_SaveDefaults, sizeof(g_SaveDefaults));

    progress->carIndex = 3;
    progress->course = 0;
    progress->classIndex = 0;
    progress->maxClassReached = -1;
    progress->money.value = 0;
}

static void ResetCourseProgressState(
    CourseProgressState *progress,
    s32 mode) {
    progress->retriesRemaining = 5;
    memset(progress->bestPlace, 0, sizeof(progress->bestPlace));

    if (mode < 2) {
        progress->bestPlace[3] = 0xFF;
    }

    progress->unlockPending = 0;
}

void ResetCourseProgress(s32 mode) {
    ResetCourseProgressState(g_CourseProgress, mode);
}

void InitSaveDefaults(void) {
    s32 i;

    memcpy(g_TimeAttackCars, g_SaveDefaults, sizeof(g_SaveDefaults));

    g_ClassRecords[0].place = 0;
    g_ClassRecords[0].clears = 0;
    g_ClassWinCount = 0;

    for (i = 1; i < 11; i++) {
        g_ClassRecords[i].place = -1;
        g_ClassRecords[i].clears = 0;
    }

    g_TimeAttackSave.course = 0;
    g_TimeAttackSave.carIndex = 3;
    g_TimeAttackSave.classIndex = 0;
    g_TimeAttackSave.maxClassReached = 0;
    g_TimeAttackSave.money.value = 0;
    ResetProgressSlot(g_GrandPrixCars, &g_GrandPrixSave);
    ResetProgressSlot(g_ExtraGrandPrixCars, &g_ExtraGrandPrixSave);

    ResetCourseProgressState(&g_ExtraGrandPrixCourseProgress, 0);
    g_CourseProgress = &g_GrandPrixCourseProgress;
    ResetCourseProgressState(g_CourseProgress, 0);

    g_MaxClassReached[1] = 0;
    g_MaxClassReached[0] = 0;
    g_BgmTrackCount = 9;
    g_BgmSelection = 0;
    ShuffleBgmOrder();
    g_BgmVolumeSetting = 0xF;
    g_SfxVolumeSetting = 0xF;
    g_MonoOutput = 0;
    ApplyAudioSettings();
}
