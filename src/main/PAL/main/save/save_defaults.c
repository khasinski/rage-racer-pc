#include "game/audio.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/grand_prix_content.h"
#include "game/save_internal.h"

#include <string.h>

enum {
    DEFAULT_PLAYER_CAR_INDEX = 3,
    DEFAULT_RETRIES_REMAINING = 5,
    DEFAULT_BGM_TRACK_COUNT = 9,
    DEFAULT_AUDIO_SETTING = 0xF,
};

void ResetProgressSlot(CarEntry *cars, GameRaceProgress *progress) {
    memcpy(cars, g_SaveDefaults, sizeof(g_SaveDefaults));

    progress->carIndex = DEFAULT_PLAYER_CAR_INDEX;
    progress->course = 0;
    progress->classIndex = 0;
    progress->maxClassReached = -1;
    progress->money = 0;
}

static void ResetCourseProgressState(
    CourseProgressState *progress,
    s32 classIndex) {
    const GrandPrixClassDefinition *definition = GrandPrixContentClass(classIndex);
    /* Preserve the legacy invalid-index fallback until save validation owns
     * rejection; valid classes derive unused slots from the content table. */
    s32 courseCount = definition ? definition->courseCount : (classIndex < 2 ? 3 : 4);
    progress->retriesRemaining = DEFAULT_RETRIES_REMAINING;
    memset(progress->bestPlace, 0, sizeof(progress->bestPlace));

    for (size_t course = (size_t)courseCount;
         course < sizeof(progress->bestPlace) / sizeof(progress->bestPlace[0]); ++course)
        progress->bestPlace[course] = 0xFF;

    progress->unlockPending = 0;
}

void ResetCourseProgress(s32 classIndex) {
    ResetCourseProgressState(g_CourseProgress, classIndex);
}

void InitSaveDefaults(void) {
    s32 i;

    memcpy(g_TimeAttackCars, g_SaveDefaults, sizeof(g_SaveDefaults));

    g_ClassRecords[0].place = 0;
    g_ClassRecords[0].clears = 0;
    g_ClassWinCount = 0;

    for (i = 1; i < CLASS_RECORD_COUNT; i++) {
        g_ClassRecords[i].place = -1;
        g_ClassRecords[i].clears = 0;
    }

    g_TimeAttackSave.course = 0;
    g_TimeAttackSave.carIndex = DEFAULT_PLAYER_CAR_INDEX;
    g_TimeAttackSave.classIndex = 0;
    g_TimeAttackSave.maxClassReached = 0;
    g_TimeAttackSave.timeAttackSeries = 0;
    ResetProgressSlot(g_GrandPrixCars, &g_GrandPrixSave);
    ResetProgressSlot(g_ExtraGrandPrixCars, &g_ExtraGrandPrixSave);

    ResetCourseProgressState(&g_ExtraGrandPrixCourseProgress, 0);
    g_CourseProgress = &g_GrandPrixCourseProgress;
    ResetCourseProgressState(g_CourseProgress, 0);

    g_MaxClassReached[1] = 0;
    g_MaxClassReached[0] = 0;
    g_BgmTrackCount = DEFAULT_BGM_TRACK_COUNT;
    g_BgmSelection = 0;
    ShuffleBgmOrder();
    g_BgmVolumeSetting = DEFAULT_AUDIO_SETTING;
    g_SfxVolumeSetting = DEFAULT_AUDIO_SETTING;
    g_MonoOutput = 0;
    ApplyAudioSettings();
}
