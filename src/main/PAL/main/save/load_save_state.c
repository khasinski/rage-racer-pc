#include "game/audio.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

static s32 ClampVolumeSetting(s32 value) {
    if (value < 0) return 0;
    return value < 0x10 ? value : 0xF;
}

static void LoadCarSetup(CarEntry *car, const SavedCarSetup *saved) {
    car->modelVariant = saved->modelVariant;
    car->tireCompound = saved->tireCompound;
    car->transmission = saved->transmission;
    car->paintColor1 = saved->paintColor1;
    car->paintColor2 = saved->paintColor2;
    car->enabled = saved->enabled;
}

s32 LoadSaveStateBlock(GameSaveBlock *block) {
    u32 checksum = CalculateSaveBlockChecksum(block);
    s32 i;

    printf("%s", g_MsgSaveChecksumOk);
    printf(g_FmtSaveChecksum, block->checksum, checksum);
    if (block->checksum != checksum) {
        return 0;
    }

    g_PadMappingIndex = block->padMappingIndex;
    g_NegconMappingIndex = block->negconMappingIndex;
    g_NegconSteerNeutral = block->negconSteerNeutral;
    g_NegconSteerPlay = block->negconSteerPlay;
    g_NegconNeutralI = block->negconNeutralI;
    g_NegconNeutralII = block->negconNeutralII;
    g_NegconNeutralL = block->negconNeutralL;
    g_NegconMaxTwist = block->negconMaxTwist;

    g_GrandPrixSave.course = block->grandPrixProgress.course;
    g_GrandPrixSave.carIndex = block->grandPrixProgress.carIndex;
    g_GrandPrixSave.classIndex = block->grandPrixProgress.classIndex;
    g_GrandPrixSave.maxClassReached = block->grandPrixProgress.maxClassReached;
    g_GrandPrixSave.money.value = block->grandPrixProgress.money;
    g_ExtraGrandPrixSave.course = block->extraGrandPrixProgress.course;
    g_ExtraGrandPrixSave.carIndex = block->extraGrandPrixProgress.carIndex;
    g_ExtraGrandPrixSave.classIndex = block->extraGrandPrixProgress.classIndex;
    g_ExtraGrandPrixSave.maxClassReached = block->extraGrandPrixProgress.maxClassReached;
    g_ExtraGrandPrixSave.money.value = block->extraGrandPrixProgress.money;
    g_TimeAttackSave.course = block->timeAttackProgress.course;
    g_TimeAttackSave.carIndex = block->timeAttackProgress.carIndex;
    g_TimeAttackSave.classIndex = block->timeAttackProgress.classIndex;
    g_TimeAttackSave.maxClassReached = block->timeAttackProgress.maxClassReached;
    g_TimeAttackSave.money.value = block->timeAttackProgress.money;
    g_BgmSelection = block->bgmSelection;
    g_ExtraGrandPrixUnlocked = block->extraGrandPrixUnlocked;
    g_MaxClassReached[0] = block->maxClassReached[0];
    g_MaxClassReached[1] = block->maxClassReached[1];

    for (i = 0; i < 13; i++) {
        LoadCarSetup(&g_GrandPrixCars[i], &block->carSetup[0][i]);
        LoadCarSetup(&g_ExtraGrandPrixCars[i], &block->carSetup[1][i]);
        LoadCarSetup(&g_TimeAttackCars[i], &block->carSetup[2][i]);
    }

    for (i = 0; i < 11; i++) {
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

    g_BgmVolumeSetting = ClampVolumeSetting(block->bgmVolume);
    g_SfxVolumeSetting = ClampVolumeSetting(block->sfxVolume);
    g_MonoOutput = block->monoOutput != 0;

    /* These fields are byte arrays in the on-disc format and typed at runtime. */
    memcpy(&g_GrandPrixCourseProgress, block->grandPrixCourseProgress, 8);
    memcpy(&g_ExtraGrandPrixCourseProgress, block->extraGrandPrixCourseProgress, 8);

    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
    ApplyAudioSettings();
    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    return 1;
}
