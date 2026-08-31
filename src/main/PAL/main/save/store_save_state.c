#include "game/race.h"
#include "game/menu.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

void StoreSaveStateBlock(GameSaveBlock *block) {
    block->padMappingIndex = g_PadMappingIndex;
    block->negconMappingIndex = g_NegconMappingIndex;
    block->negconSteerNeutral = g_NegconSteerNeutral;
    block->negconSteerPlay = g_NegconSteerPlay;
    block->negconNeutralI = g_NegconNeutralI;
    block->negconNeutralII = g_NegconNeutralII;
    block->negconMaxTwist = g_NegconMaxTwist;
    block->negconNeutralL = g_NegconNeutralL;

    block->grandPrixProgress.course = g_GrandPrixSave.course;
    block->grandPrixProgress.carIndex = g_GrandPrixSave.carIndex;
    block->grandPrixProgress.classIndex = g_GrandPrixSave.classIndex;
    block->grandPrixProgress.maxClassReached = g_GrandPrixSave.maxClassReached;
    block->grandPrixProgress.money = g_GrandPrixSave.money.value;
    block->extraGrandPrixProgress.course = g_ExtraGrandPrixSave.course;
    block->extraGrandPrixProgress.carIndex = g_ExtraGrandPrixSave.carIndex;
    block->extraGrandPrixProgress.classIndex = g_ExtraGrandPrixSave.classIndex;
    block->extraGrandPrixProgress.maxClassReached = g_ExtraGrandPrixSave.maxClassReached;
    block->extraGrandPrixProgress.money = g_ExtraGrandPrixSave.money.value;
    block->timeAttackProgress.course = g_TimeAttackSave.course;
    block->timeAttackProgress.carIndex = g_TimeAttackSave.carIndex;
    block->timeAttackProgress.classIndex = g_TimeAttackSave.classIndex;
    block->timeAttackProgress.maxClassReached = g_TimeAttackSave.maxClassReached;
    block->timeAttackProgress.money = g_TimeAttackSave.money.value;
    block->bgmSelection = g_BgmSelection;
    block->extraGrandPrixUnlocked = g_ExtraGrandPrixUnlocked;
    block->maxClassReached[0] = g_MaxClassReached[0];
    block->maxClassReached[1] = g_MaxClassReached[1];

    {
        s32 i;

        for (i = 0; i < 13; i++) {
                SavedCarSetup *grandPrixCar = &block->carSetup[0][i];
                SavedCarSetup *extraGrandPrixCar = &block->carSetup[1][i];
                SavedCarSetup *timeAttackCar = &block->carSetup[2][i];

                grandPrixCar->modelVariant = g_GrandPrixCars[i].modelVariant;
                grandPrixCar->tireCompound = g_GrandPrixCars[i].tireCompound;
                grandPrixCar->transmission = g_GrandPrixCars[i].transmission;
                grandPrixCar->paintColor1 = g_GrandPrixCars[i].paintColor1;
                grandPrixCar->paintColor2 = g_GrandPrixCars[i].paintColor2;
                grandPrixCar->enabled = g_GrandPrixCars[i].enabled;

                extraGrandPrixCar->modelVariant = g_ExtraGrandPrixCars[i].modelVariant;
                extraGrandPrixCar->tireCompound = g_ExtraGrandPrixCars[i].tireCompound;
                extraGrandPrixCar->transmission = g_ExtraGrandPrixCars[i].transmission;
                extraGrandPrixCar->paintColor1 = g_ExtraGrandPrixCars[i].paintColor1;
                extraGrandPrixCar->paintColor2 = g_ExtraGrandPrixCars[i].paintColor2;
                extraGrandPrixCar->enabled = g_ExtraGrandPrixCars[i].enabled;

                timeAttackCar->modelVariant = g_TimeAttackCars[i].modelVariant;
                timeAttackCar->tireCompound = g_TimeAttackCars[i].tireCompound;
                timeAttackCar->transmission = g_TimeAttackCars[i].transmission;
                timeAttackCar->paintColor1 = g_TimeAttackCars[i].paintColor1;
                timeAttackCar->paintColor2 = g_TimeAttackCars[i].paintColor2;
                timeAttackCar->enabled = g_TimeAttackCars[i].enabled;

        }
    }
    {
        s32 index;

        for (index = 0; index < 11; index++) {
            SavedClassRecord *dst = &block->classRecords[index];
            dst->grade = g_ClassRecords[index].place;
            dst->clears = g_ClassRecords[index].clears;
        }
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

    {
        /* These two accumulator hints are load-bearing. */
        u32 count = 0;
        u32 checksum = 0;
        GameSaveBlockAddress checksumAddress;
        u16 *checksumSrc;

        checksumAddress.pointer = block;
        checksumSrc = checksumAddress.halfwordPointer;

        block->bgmVolume = g_BgmVolumeSetting;
        block->sfxVolume = g_SfxVolumeSetting;
        block->monoOutput = g_MonoOutput;
        memcpy(block->grandPrixCourseProgress, &g_GrandPrixCourseProgress, 8);
        memcpy(block->extraGrandPrixCourseProgress, &g_ExtraGrandPrixCourseProgress, 8);

        for (; count < 0x7FE; count++) {
            checksum += *checksumSrc++;
        }
        block->checksum = ~checksum;
    }
}
