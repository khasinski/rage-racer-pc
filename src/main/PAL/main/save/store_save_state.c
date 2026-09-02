#include "game/audio.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

#include <string.h>

static void StoreCarSetup(SavedCarSetup *saved, const CarEntry *car) {
    saved->modelVariant = car->modelVariant;
    saved->tireCompound = car->tireCompound;
    saved->transmission = car->transmission;
    saved->paintColor1 = car->paintColor1;
    saved->paintColor2 = car->paintColor2;
    saved->enabled = car->enabled;
}

static void StoreRaceProgress(
    SavedRaceProgress *saved,
    const GameRaceProgress *progress) {
    saved->course = progress->course;
    saved->carIndex = progress->carIndex;
    saved->classIndex = progress->classIndex;
    saved->maxClassReached = progress->maxClassReached;
    saved->money = progress->money.value;
}

void StoreSaveStateBlock(GameSaveBlock *block) {
    s32 i;

    memset(block, 0, sizeof(*block));

    block->padMappingIndex = g_PadMappingIndex;
    block->negconMappingIndex = g_NegconMappingIndex;
    block->negconSteerNeutral = g_NegconSteerNeutral;
    block->negconSteerPlay = g_NegconSteerPlay;
    block->negconNeutralI = g_NegconNeutralI;
    block->negconNeutralII = g_NegconNeutralII;
    block->negconMaxTwist = g_NegconMaxTwist;
    block->negconNeutralL = g_NegconNeutralL;

    StoreRaceProgress(&block->grandPrixProgress, &g_GrandPrixSave);
    StoreRaceProgress(&block->extraGrandPrixProgress, &g_ExtraGrandPrixSave);
    StoreRaceProgress(&block->timeAttackProgress, &g_TimeAttackSave);
    block->bgmSelection = g_BgmSelection;
    block->extraGrandPrixUnlocked = g_ExtraGrandPrixUnlocked;
    block->maxClassReached[0] = g_MaxClassReached[0];
    block->maxClassReached[1] = g_MaxClassReached[1];

    for (i = 0; i < GAME_CAR_COUNT; i++) {
        StoreCarSetup(&block->carSetup[SAVED_CARS_GRAND_PRIX][i],
                      &g_GrandPrixCars[i]);
        StoreCarSetup(&block->carSetup[SAVED_CARS_EXTRA_GRAND_PRIX][i],
                      &g_ExtraGrandPrixCars[i]);
        StoreCarSetup(&block->carSetup[SAVED_CARS_TIME_ATTACK][i],
                      &g_TimeAttackCars[i]);
    }

    for (i = 0; i < CLASS_RECORD_COUNT; i++) {
        block->classRecords[i].grade = g_ClassRecords[i].place;
        block->classRecords[i].clears = g_ClassRecords[i].clears;
    }

    memcpy(block->teamLogoClut, g_TeamLogoClut, sizeof(block->teamLogoClut));
    memcpy(block->teamLogoCanvas, g_TeamLogoCanvas.halfwords,
           sizeof(block->teamLogoCanvas));
    memcpy(block->bestLapTimes, g_BestLapTimes, sizeof(block->bestLapTimes));
    memcpy(block->bestTotalTimes, g_BestTotalTimes,
           sizeof(block->bestTotalTimes));
    memcpy(block->rankingRecords, g_RankingRecords,
           sizeof(block->rankingRecords));
    memcpy(block->timeRecords, g_TimeRecords, sizeof(block->timeRecords));
    memcpy(block->bestSectorTimes, g_BestSectorTimes,
           sizeof(block->bestSectorTimes));

    block->bgmVolume = g_BgmVolumeSetting;
    block->sfxVolume = g_SfxVolumeSetting;
    block->monoOutput = g_MonoOutput;
    memcpy(block->grandPrixCourseProgress, &g_GrandPrixCourseProgress,
           sizeof(g_GrandPrixCourseProgress));
    memcpy(block->extraGrandPrixCourseProgress,
           &g_ExtraGrandPrixCourseProgress,
           sizeof(g_ExtraGrandPrixCourseProgress));
    block->checksum = CalculateSaveBlockChecksum(block);
}
