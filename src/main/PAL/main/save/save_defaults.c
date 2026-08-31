#include "game/audio.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"

void ResetProgressSlot(CarEntry *slot, GameRaceProgress *progress) {
    s32 i;

    for (i = 0; i < 13; i++) {
        slot[i] = g_SaveDefaults[i];
    }

    progress->carIndex = 3;
    progress->course = 0;
    progress->classIndex = 0;
    progress->maxClassReached = -1;
    progress->money.value = 0;
}

void ResetCourseProgress(s32 mode) {
    CourseProgressState *progress = g_CourseProgress;

    progress->retriesRemaining = 5;
    progress->bestPlace[3] = 0;
    progress->bestPlace[2] = 0;
    progress->bestPlace[1] = 0;
    progress->bestPlace[0] = 0;

    if (mode < 2) {
        g_CourseProgress->bestPlace[3] = 0xFF;
    }

    g_CourseProgress->unlockPending = 0;
}

void InitSaveDefaults(void) {
    s32 i;
    s32 emptySlot;

    for (i = 0; i < 13; i++) {
        g_TimeAttackCars[i] = g_SaveDefaults[i];
    }

    g_ClassRecords[0].place = 0;
    g_ClassRecords[0].clears = 0;
    g_ClassWinCount = 0;

    emptySlot = -1;
    for (i = 1; i < 11; i++) {
        g_ClassRecords[i].place = emptySlot;
        g_ClassRecords[i].clears = 0;
    }

    g_TimeAttackSave.course = 0;
    g_TimeAttackSave.carIndex = 3;
    g_TimeAttackSave.classIndex = 0;
    g_TimeAttackSave.maxClassReached = 0;
    g_TimeAttackSave.money.value = 0;
    ResetProgressSlot(g_GrandPrixCars, &g_GrandPrixSave);
    ResetProgressSlot(g_ExtraGrandPrixCars, &g_ExtraGrandPrixSave);

    g_CourseProgress = &g_ExtraGrandPrixCourseProgress;
    ResetCourseProgress(0);
    g_CourseProgress = &g_GrandPrixCourseProgress;
    ResetCourseProgress(0);

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
