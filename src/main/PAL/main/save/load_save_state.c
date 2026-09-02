#include "game/audio.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

static void LoadCarSetup(CarEntry *car, const SavedCarSetup *saved) {
    car->modelVariant = saved->modelVariant;
    car->tireCompound = saved->tireCompound;
    car->transmission = saved->transmission;
    car->paintColor1 = saved->paintColor1;
    car->paintColor2 = saved->paintColor2;
    car->enabled = saved->enabled;
}

static void LoadRaceProgress(
    GameRaceProgress *progress,
    const SavedRaceProgress *saved) {
    progress->course = saved->course;
    progress->carIndex = saved->carIndex;
    progress->classIndex = saved->classIndex;
    progress->maxClassReached = saved->maxClassReached;
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
    g_ExtraGrandPrixUnlocked = block->extraGrandPrixUnlocked;
    g_MaxClassReached[0] = block->maxClassReached[0];
    g_MaxClassReached[1] = block->maxClassReached[1];

    for (i = 0; i < GAME_CAR_COUNT; i++) {
        LoadCarSetup(&g_GrandPrixCars[i], &block->carSetup[0][i]);
        LoadCarSetup(&g_ExtraGrandPrixCars[i], &block->carSetup[1][i]);
        LoadCarSetup(&g_TimeAttackCars[i], &block->carSetup[2][i]);
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
