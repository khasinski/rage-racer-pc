#include "game/audio.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

GameRaceProgress g_GrandPrixSave;
GameRaceProgress g_ExtraGrandPrixSave;
GameRaceProgress g_TimeAttackSave;

static s32 s_shuffleCalls;
static s32 s_audioApplyCalls;

void ShuffleBgmOrder(void) { s_shuffleCalls++; }
void ApplyAudioSettings(void) { s_audioApplyCalls++; }

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int TestProgressSlotReset(void) {
    CarEntry cars[13];
    GameRaceProgress progress;

    memset(cars, 0xA5, sizeof(cars));
    memset(&progress, 0xA5, sizeof(progress));
    ResetProgressSlot(cars, &progress);

    CHECK(memcmp(cars, g_SaveDefaults, sizeof(cars)) == 0);
    CHECK(progress.course == 0);
    CHECK(progress.carIndex == 3);
    CHECK(progress.classIndex == 0);
    CHECK(progress.maxClassReached == -1);
    CHECK(progress.money.value == 0);
    return 0;
}

static int TestCourseProgressModes(void) {
    CourseProgressState progress;

    memset(&progress, 0xA5, sizeof(progress));
    g_CourseProgress = &progress;
    ResetCourseProgress(0);
    CHECK(progress.retriesRemaining == 5);
    CHECK(progress.unlockPending == 0);
    CHECK(progress.bestPlace[0] == 0 && progress.bestPlace[1] == 0);
    CHECK(progress.bestPlace[2] == 0 && progress.bestPlace[3] == 0xFF);

    memset(&progress, 0xA5, sizeof(progress));
    ResetCourseProgress(2);
    CHECK(progress.bestPlace[0] == 0 && progress.bestPlace[1] == 0);
    CHECK(progress.bestPlace[2] == 0 && progress.bestPlace[3] == 0);
    return 0;
}

static int TestAllDefaults(void) {
    s32 i;

    memset(g_GrandPrixCars, 0xA5, 13 * sizeof(*g_GrandPrixCars));
    memset(g_ExtraGrandPrixCars, 0xA5, 13 * sizeof(*g_ExtraGrandPrixCars));
    memset(g_TimeAttackCars, 0xA5, 13 * sizeof(*g_TimeAttackCars));
    memset(g_ClassRecords, 0xA5, sizeof(g_ClassRecords));
    memset(&g_GrandPrixCourseProgress, 0xA5,
           sizeof(g_GrandPrixCourseProgress));
    memset(&g_ExtraGrandPrixCourseProgress, 0xA5,
           sizeof(g_ExtraGrandPrixCourseProgress));

    InitSaveDefaults();

    CHECK(memcmp(g_GrandPrixCars, g_SaveDefaults,
                 sizeof(g_SaveDefaults)) == 0);
    CHECK(memcmp(g_ExtraGrandPrixCars, g_SaveDefaults,
                 sizeof(g_SaveDefaults)) == 0);
    CHECK(memcmp(g_TimeAttackCars, g_SaveDefaults,
                 sizeof(g_SaveDefaults)) == 0);
    CHECK(g_ClassRecords[0].place == 0 && g_ClassRecords[0].clears == 0);
    for (i = 1; i < 11; i++) {
        CHECK(g_ClassRecords[i].place == -1);
        CHECK(g_ClassRecords[i].clears == 0);
    }
    CHECK(g_CourseProgress == &g_GrandPrixCourseProgress);
    CHECK(g_GrandPrixCourseProgress.bestPlace[3] == 0xFF);
    CHECK(g_ExtraGrandPrixCourseProgress.bestPlace[3] == 0xFF);
    CHECK(g_TimeAttackSave.carIndex == 3);
    CHECK(g_TimeAttackSave.maxClassReached == 0);
    CHECK(g_GrandPrixSave.maxClassReached == -1);
    CHECK(g_ExtraGrandPrixSave.maxClassReached == -1);
    CHECK(g_BgmTrackCount == 9 && g_BgmSelection == 0);
    CHECK(g_BgmVolumeSetting == 0xF && g_SfxVolumeSetting == 0xF);
    CHECK(g_MonoOutput == 0);
    CHECK(s_shuffleCalls == 1 && s_audioApplyCalls == 1);
    return 0;
}

int main(void) {
    if (TestProgressSlotReset() != 0) return 1;
    if (TestCourseProgressModes() != 0) return 1;
    if (TestAllDefaults() != 0) return 1;
    puts("save defaults reset cars, progress, records and audio settings");
    return 0;
}
