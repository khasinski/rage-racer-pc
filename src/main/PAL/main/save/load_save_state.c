#include "game/audio.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/input_internal.h"
#include "game/prize_money.h"
#include "game/save_internal.h"

enum { MAX_SAVED_BGM_SELECTION = 10 };

static s32 ClampSaveValue(s32 value, s32 minimum, s32 maximum) {
    if (value < minimum) return minimum;
    return value > maximum ? maximum : value;
}

static u8 ClampCarModelVariant(s32 carIndex, u8 variant) {
    s32 firstVariant = g_CarModelBaseIndex[carIndex];
    s32 nextVariant = carIndex + 1 < GAME_CAR_COUNT
                          ? g_CarModelBaseIndex[carIndex + 1]
                          : CAR_MODEL_VARIANT_COUNT;
    s32 variantCount = nextVariant - firstVariant;

    return variant < variantCount ? variant : 0;
}

static void LoadCarSetup(CarEntry *car, const SavedCarSetup *saved,
                         s32 carIndex) {
    car->modelVariant = ClampCarModelVariant(carIndex, saved->modelVariant);
    car->tireCompound = (u8)ClampSaveValue(
        saved->tireCompound, 0, CAR_TIRE_COMPOUND_COUNT - 1);
    car->transmission = saved->transmission != 0;
    car->paintColor1 = (u8)ClampSaveValue(
        saved->paintColor1, 0, MENU_PAINT_COLOR_COUNT - 1);
    car->paintColor2 = (u8)ClampSaveValue(
        saved->paintColor2, 0, MENU_PAINT_COLOR_COUNT - 1);
    car->enabled = saved->enabled != 0;
}

static void LoadRaceProgressFields(
    GameRaceProgress *progress,
    const SavedRaceProgress *saved) {
    progress->course = ClampSaveValue(saved->course, 0, COURSE_LONG_SLOT);
    progress->carIndex = ClampSaveValue(
        saved->carIndex, 0, GAME_CAR_COUNT - 1);
    progress->classIndex = ClampSaveValue(
        saved->classIndex, 0, GRAND_PRIX_FINAL_CLASS_INDEX);
    progress->maxClassReached = ClampSaveValue(
        saved->maxClassReached, -1, GRAND_PRIX_FINAL_CLASS_INDEX);
}

static void LoadGrandPrixProgress(GameRaceProgress *progress,
                                  const SavedRaceProgress *saved) {
    LoadRaceProgressFields(progress, saved);
    progress->money = ClampPrizeMoney(saved->money);
}

static void LoadTimeAttackProgress(GameRaceProgress *progress,
                                   const SavedRaceProgress *saved) {
    LoadRaceProgressFields(progress, saved);
    progress->timeAttackSeries = ClampSaveValue(
        saved->timeAttackSeries, 0, RECORD_SERIES_COUNT - 1);
}

static void NormalizeRaceRecords(
    RaceRecord records[RECORD_SERIES_COUNT][RECORD_COURSE_COUNT]
                      [RECORD_TABLE_LENGTH]) {
    s32 series;
    s32 course;
    s32 row;

    for (series = 0; series < RECORD_SERIES_COUNT; series++) {
        for (course = 0; course < RECORD_COURSE_COUNT; course++) {
            for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
                RaceRecord *record = &records[series][course][row];
                s32 character;

                record->carIndex = (s16)ClampSaveValue(
                    record->carIndex, 0, GAME_CAR_COUNT - 1);
                for (character = 0; character < RECORD_NAME_LENGTH;
                     character++) {
                    u8 value = (u8)record->driverName[character];

                    if (value < ' ' || value > '~') {
                        record->driverName[character] = '?';
                    }
                }
                record->driverName[RECORD_NAME_LENGTH] = '\0';
                record->driverName[RECORD_NAME_LENGTH + 1] = '\0';
            }
        }
    }
}

static void NormalizeCourseProgress(CourseProgressState *progress) {
    s32 course;

    for (course = 0; course <= COURSE_LONG_SLOT; course++) {
        u8 place = progress->bestPlace[course];

        if (place > 3 && place != 0xFF) {
            progress->bestPlace[course] = 0;
        }
    }
    progress->unlockPending = progress->unlockPending != 0;
    progress->retriesRemaining = (s16)ClampSaveValue(
        progress->retriesRemaining, 0, 5);
}

s32 LoadSaveStateBlock(const GameSaveBlock *block) {
    u32 checksum = CalculateSaveBlockChecksum(block);
    s32 i;

    if (block->checksum != checksum) {
        return 0;
    }

    g_PadMappingIndex =
        ClampControllerMappingIndex(block->padMappingIndex);
    g_NegconMappingIndex =
        ClampControllerMappingIndex(block->negconMappingIndex);
    g_NegconSteerNeutral = block->negconSteerNeutral;
    g_NegconSteerPlay =
        ClampNegconCalibrationValue(block->negconSteerPlay);
    g_NegconNeutralI = block->negconNeutralI;
    g_NegconNeutralII = block->negconNeutralII;
    g_NegconNeutralL = block->negconNeutralL;
    g_NegconMaxTwist =
        ClampNegconCalibrationValue(block->negconMaxTwist);

    LoadGrandPrixProgress(&g_GrandPrixSave, &block->grandPrixProgress);
    LoadGrandPrixProgress(&g_ExtraGrandPrixSave,
                          &block->extraGrandPrixProgress);
    LoadTimeAttackProgress(&g_TimeAttackSave, &block->timeAttackProgress);
    g_BgmSelection = ClampSaveValue(
        block->bgmSelection, 0, MAX_SAVED_BGM_SELECTION);
    g_ExtraGrandPrixUnlocked = block->extraGrandPrixUnlocked != 0;
    g_MaxClassReached[0] = ClampSaveValue(
        block->maxClassReached[0], 0, GRAND_PRIX_FINAL_CLASS_INDEX);
    g_MaxClassReached[1] = ClampSaveValue(
        block->maxClassReached[1], 0, GRAND_PRIX_FINAL_CLASS_INDEX);

    for (i = 0; i < GAME_CAR_COUNT; i++) {
        LoadCarSetup(&g_GrandPrixCars[i],
                     &block->carSetup[SAVED_CARS_GRAND_PRIX][i], i);
        LoadCarSetup(&g_ExtraGrandPrixCars[i],
                     &block->carSetup[SAVED_CARS_EXTRA_GRAND_PRIX][i], i);
        LoadCarSetup(&g_TimeAttackCars[i],
                     &block->carSetup[SAVED_CARS_TIME_ATTACK][i], i);
    }

    for (i = 0; i < CLASS_RECORD_COUNT; i++) {
        s16 place = (s16)block->classRecords[i].grade;

        g_ClassRecords[i].place = (s16)ClampSaveValue(place, -1, 3);
        g_ClassRecords[i].clears = (u16)ClampSaveValue(
            block->classRecords[i].clears, 0, 99);
    }

    memcpy(g_TeamLogoClut, block->teamLogoClut, sizeof(block->teamLogoClut));
    memcpy(g_TeamLogoCanvas.halfwords, block->teamLogoCanvas,
           sizeof(block->teamLogoCanvas));
    memcpy(g_BestLapTimes, block->bestLapTimes, sizeof(g_BestLapTimes));
    memcpy(g_BestTotalTimes, block->bestTotalTimes,
           sizeof(g_BestTotalTimes));
    memcpy(g_RankingRecords, block->rankingRecords,
           sizeof(block->rankingRecords));
    memcpy(g_TimeRecords, block->timeRecords, sizeof(block->timeRecords));
    NormalizeRaceRecords(g_RankingRecords);
    NormalizeRaceRecords(g_TimeRecords);
    memcpy(g_BestSectorTimes, block->bestSectorTimes,
           sizeof(block->bestSectorTimes));
    RepairRecordTimes();

    g_BgmVolumeSetting = ClampAudioSetting(block->bgmVolume);
    g_SfxVolumeSetting = ClampAudioSetting(block->sfxVolume);
    g_MonoOutput = block->monoOutput != 0;

    /* These fields are byte arrays in the on-disc format and typed at runtime. */
    memcpy(&g_GrandPrixCourseProgress, block->grandPrixCourseProgress,
           sizeof(g_GrandPrixCourseProgress));
    memcpy(&g_ExtraGrandPrixCourseProgress,
           block->extraGrandPrixCourseProgress,
           sizeof(g_ExtraGrandPrixCourseProgress));
    NormalizeCourseProgress(&g_GrandPrixCourseProgress);
    NormalizeCourseProgress(&g_ExtraGrandPrixCourseProgress);

    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
    ApplyAudioSettings();
    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    return 1;
}
