#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

void StoreSaveStateBlock(GameSaveBlock *block) {
    register GameSaveBlockAddress saveAddress asm("$4");
    {
        GameSaveBlockAddress padMappingAddress;
        GameSaveBlockAddress negconMappingAddress;
        GameSaveBlockAddress steerNeutralAddress;
        GameSaveBlockAddress steerPlayAddress;
        u16 padMappingIndex = g_PadMappingIndex;
        u16 negconMappingIndex = g_NegconMappingIndex;
        u16 negconSteerNeutral = g_NegconSteerNeutral;
        u16 negconSteerPlay = g_NegconSteerPlay;
        padMappingAddress.halfwordPointer = &block->padMappingIndex;
        negconMappingAddress.halfwordPointer = &block->negconMappingIndex;
        steerNeutralAddress.halfwordPointer = &block->negconSteerNeutral;
        steerPlayAddress.halfwordPointer = &block->negconSteerPlay;
        *padMappingAddress.halfwordPointer = padMappingIndex;
        *negconMappingAddress.halfwordPointer = negconMappingIndex;
        *steerNeutralAddress.halfwordPointer = negconSteerNeutral;
        *steerPlayAddress.halfwordPointer = negconSteerPlay;
    }
    {
        GameSaveBlockAddress neutralIAddress;
        GameSaveBlockAddress neutralIIAddress;

        neutralIAddress.halfwordPointer = &block->negconNeutralI;
        neutralIIAddress.halfwordPointer = &block->negconNeutralII;
        *neutralIAddress.halfwordPointer = g_NegconNeutralI;
        *neutralIIAddress.halfwordPointer = g_NegconNeutralII;
    }
    {
        u16 negconMaxTwist = g_NegconMaxTwist;
        u16 negconNeutralL = g_NegconNeutralL;
        block->negconMaxTwist = negconMaxTwist;
        block->negconNeutralL = negconNeutralL;
    }

    block->grandPrixProgress.course = g_GrandPrixSave.course;
    block->grandPrixProgress.carIndex = g_GrandPrixSave.carIndex;
    block->grandPrixProgress.classIndex = g_GrandPrixSave.classIndex;
    block->grandPrixProgress.maxClassReached = g_GrandPrixSave.maxClassReached;
    block->grandPrixProgress.money = g_GrandPrixSave.money.value;
    block->extraGrandPrixProgress.course = g_ExtraGrandPrixSave.course;
    block->extraGrandPrixProgress.carIndex = g_ExtraGrandPrixSave.carIndex;
    block->extraGrandPrixProgress.classIndex = g_ExtraGrandPrixSave.classIndex;
    block->extraGrandPrixProgress.maxClassReached = g_ExtraGrandPrixSave.maxClassReached;
    {
        s32 extraMoney = g_ExtraGrandPrixSave.money.value;
        u16 bgmSelection = g_BgmSelection;
        block->extraGrandPrixProgress.money = extraMoney;
        block->timeAttackProgress.course = g_TimeAttackSave.course;
        block->timeAttackProgress.carIndex = g_TimeAttackSave.carIndex;
        block->timeAttackProgress.classIndex = g_TimeAttackSave.classIndex;
        {
            GameSaveBlockAddress maxClassAddress;

            maxClassAddress.wordPointer =
                &block->timeAttackProgress.maxClassReached;
            *maxClassAddress.wordPointer = g_TimeAttackSave.maxClassReached;
        }
        {
            u16 extraGrandPrixUnlocked = g_ExtraGrandPrixUnlocked;
            saveAddress.offset = g_TimeAttackSave.money.value;
            block->bgmSelection = bgmSelection;
            block->extraGrandPrixUnlocked = extraGrandPrixUnlocked;
            block->timeAttackProgress.money = saveAddress.offset;
        }
    }
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
        register u32 count asm("$13") = 0;
        register u32 checksum asm("$6") = 0;
        s32 bgmVolume = g_BgmVolumeSetting;
        s32 sfxVolume = g_SfxVolumeSetting;
        s32 monoOutput = g_MonoOutput;
        GameSaveBlockAddress checksumAddress;
        u16 *checksumSrc;

        checksumAddress.pointer = block;
        checksumSrc = checksumAddress.halfwordPointer;

        block->bgmVolume = bgmVolume;
        block->sfxVolume = sfxVolume;
        block->monoOutput = monoOutput;
        memcpy(block->grandPrixCourseProgress, &g_GrandPrixCourseProgress, 8);
        memcpy(block->extraGrandPrixCourseProgress, &g_ExtraGrandPrixCourseProgress, 8);

        for (; count < 0x7FE; count++) {
            checksum += *checksumSrc++;
        }
        block->checksum = ~checksum;
    }
}
