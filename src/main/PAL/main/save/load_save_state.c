#include "game/audio.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

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

static void LoadRaceProgress(
    GameRaceProgress *progress,
    const SavedRaceProgress *saved) {
    progress->course = ClampSaveValue(saved->course, 0, COURSE_LONG_SLOT);
    progress->carIndex = ClampSaveValue(
        saved->carIndex, 0, GAME_CAR_COUNT - 1);
    progress->classIndex = ClampSaveValue(
        saved->classIndex, 0, GRAND_PRIX_FINAL_CLASS_INDEX);
    progress->maxClassReached = ClampSaveValue(
        saved->maxClassReached, -1, GRAND_PRIX_FINAL_CLASS_INDEX);
    progress->money.value = saved->money;
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
    g_NegconSteerPlay = block->negconSteerPlay;
    g_NegconNeutralI = block->negconNeutralI;
    g_NegconNeutralII = block->negconNeutralII;
    g_NegconNeutralL = block->negconNeutralL;
    g_NegconMaxTwist = block->negconMaxTwist;

    LoadRaceProgress(&g_GrandPrixSave, &block->grandPrixProgress);
    LoadRaceProgress(&g_ExtraGrandPrixSave, &block->extraGrandPrixProgress);
    LoadRaceProgress(&g_TimeAttackSave, &block->timeAttackProgress);
    g_BgmSelection = block->bgmSelection;
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
        g_ClassRecords[i].place = block->classRecords[i].grade;
        g_ClassRecords[i].clears = block->classRecords[i].clears;
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

    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
    ApplyAudioSettings();
    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    return 1;
}
