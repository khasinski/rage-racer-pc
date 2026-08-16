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
        {
            s32 i;

            saveAddress.pointer = block;
            for (i = 0; i < 13; i++) {
                SavedCarSetup *grandPrixCar =
                    &saveAddress.pointer->carSetup[0][0];
                SavedCarSetup *extraGrandPrixCar =
                    &saveAddress.pointer->carSetup[1][0];
                SavedCarSetup *timeAttackCar =
                    &saveAddress.pointer->carSetup[2][0];

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

                saveAddress.bytePointer += sizeof(SavedCarSetup);
            }
        }

        {
            s32 index;

            saveAddress.pointer = block;
            index = 0;
            for (; index < 11; index++) {
                SavedClassRecord *dst =
                    &saveAddress.pointer->classRecords[0];
                dst->grade = g_ClassRecords[index].place;
                dst->clears = g_ClassRecords[index].clears;
                saveAddress.bytePointer += sizeof(SavedClassRecord);
            }
        }
    }

    {
        register s32 count asm("$13");

        {
            u16 *src;
            GameSaveBlockAddress dstAddress;

            count = 0;
            src = g_TeamLogoClut;
            dstAddress.pointer = block;
            for (; count < 0x10; count++) {
                dstAddress.pointer->teamLogoClut[0] = *src++;
                dstAddress.halfwordPointer++;
            }
        }

        {
            u16 *src;
            GameSaveBlockAddress dstAddress;

            count = 0;
            src = g_TeamLogoCanvas.halfwords;
            dstAddress.pointer = block;
            for (; count < 0x400; count++) {
                dstAddress.pointer->teamLogoCanvas[0] = *src++;
                dstAddress.halfwordPointer++;
            }
        }
    }

    {
        s32 inner;
        register s32 middle asm("$12");

        {
            /* The remaining register hints in these loops are load-bearing. */
            s32 outer = 0;
            s32 *lapBase = &g_BestLapTimes[0][0][0];
            s32 *totalBase = &g_BestTotalTimes[0][0][0];
            GameSaveBlockAddress outerDestinationAddress;
            u8 *outerDst;

            outerDestinationAddress.pointer = block;
            outerDst = outerDestinationAddress.bytePointer;

            for (; outer < 2; outer++) {
                register s32 outerOffset asm("$9") = (middle = 0, outer * 32);
                register u8 *middleDst asm("$11") = outerDst;
                GameSaveBlockAddress lapBlockAddress;
                LapTimeTableAddress lapDestinationAddress;
                u8 *lapDst;

                lapBlockAddress.bytePointer = outerDst;
                lapDestinationAddress.pointer =
                    &lapBlockAddress.pointer->bestLapTimes[0][0][0];
                lapDst = lapDestinationAddress.bytes;

            for (; middle < 4; middle++) {
                LapTimeTableAddress totalOutputBaseAddress;
                LapTimeTableAddress totalOutputAddress;
                LapTimeTableAddress totalInputBaseAddress;
                LapTimeTableAddress totalInputOuterAddress;
                LapTimeTableAddress totalInputAddress;
                LapTimeTableAddress lapOutputAddress;
                LapTimeTableAddress lapInputBaseAddress;
                LapTimeTableAddress lapInputOuterAddress;
                LapTimeTableAddress lapInputAddress;
                GameSaveBlockAddress middleDestinationAddress;
                s32 middleOffset = (inner = 0, middle * 8);
                s32 *totalOut;
                s32 *totalIn;
                s32 *lapOut;
                s32 *lapIn;

                middleDestinationAddress.bytePointer = middleDst;
                totalOutputBaseAddress.pointer =
                    &middleDestinationAddress.pointer->bestTotalTimes[0][0][0];
                totalOutputAddress.bytes =
                    totalOutputBaseAddress.bytes + middleOffset;
                totalOut = totalOutputAddress.pointer;
                totalInputBaseAddress.pointer = totalBase;
                totalInputOuterAddress.bytes =
                    totalInputBaseAddress.bytes + outerOffset;
                totalInputAddress.bytes =
                    totalInputOuterAddress.bytes + middleOffset;
                totalIn = totalInputAddress.pointer;
                lapOutputAddress.bytes = lapDst;
                lapOut = lapOutputAddress.pointer;
                lapInputBaseAddress.pointer = lapBase;
                lapInputOuterAddress.bytes =
                    lapInputBaseAddress.bytes + outerOffset;
                lapInputAddress.bytes =
                    lapInputOuterAddress.bytes + middleOffset;
                lapIn = lapInputAddress.pointer;

                for (; inner < 2; inner++) {
                    *lapOut = *lapIn++;
                    *totalOut++ = *totalIn++;
                    lapOut++;
                }
                lapDst += sizeof(g_BestLapTimes[0][0]);
            }
                outerDst += sizeof(g_BestLapTimes[0]);
            }
        }

        {
        /* The remaining register hints in these loops are load-bearing. */
        s32 outer = 0;
        s32 *rankingBase = GetRaceRecordWords(&g_RankingRecords[0][0][0]);
        s32 *timeBase = GetRaceRecordWords(&g_TimeRecords[0][0][0]);
        GameSaveBlockAddress outerDestinationAddress;
        register u8 *outerDst asm("$25") =
            (outerDestinationAddress.pointer = block,
             outerDestinationAddress.bytePointer);
        register s32 outerOffset asm("$16") = 0;

        for (; outer < 2; outer++) {
            s32 middle = 0;
            register s32 currentOuterOffset asm("$15") = outerOffset;
            register u8 *middleDst asm("$17") = outerDst;
            GameSaveBlockAddress outerBlockAddress;
            RaceRecordAddress rankingDestinationAddress;
            register u8 *rankingDst asm("$14") =
                (outerBlockAddress.bytePointer = outerDst,
                 rankingDestinationAddress.pointer =
                     &outerBlockAddress.pointer->rankingRecords[0][0][0],
                 rankingDestinationAddress.bytePointer);
            s32 middleOffset = 0;

            for (; middle < 4; middle++) {
                RaceRecordAddress timeDestinationBase;
                RaceRecordAddress timeDestinationAddress;
                RaceRecordAddress timeInputBaseAddress;
                RaceRecordAddress timeInputOuterAddress;
                RaceRecordAddress timeInputAddress;
                RaceRecordAddress rankingInputBaseAddress;
                RaceRecordAddress rankingInputOuterAddress;
                RaceRecordAddress rankingInputAddress;
                RaceRecordAddress rankingOutputAddress;
                GameSaveBlockAddress middleBlockAddress;
                s32 *timeDst;
                s32 *timeIn;
                s32 *rankingOut;
                s32 *rankingIn;
                middleBlockAddress.bytePointer = middleDst;
                timeDestinationBase.pointer =
                    (inner = 0, &middleBlockAddress.pointer->timeRecords[0][0][0]);
                timeDestinationAddress.bytePointer =
                    timeDestinationBase.bytePointer + middleOffset;
                timeDst = timeDestinationAddress.wordPointer;
                timeInputBaseAddress.wordPointer = timeBase;
                timeInputOuterAddress.bytePointer =
                    timeInputBaseAddress.bytePointer + currentOuterOffset;
                timeInputAddress.bytePointer =
                    timeInputOuterAddress.bytePointer + middleOffset;
                timeIn = timeInputAddress.wordPointer;
                rankingOutputAddress.bytePointer = rankingDst;
                rankingOut = rankingOutputAddress.wordPointer;
                rankingInputBaseAddress.wordPointer = rankingBase;
                rankingInputOuterAddress.bytePointer =
                    rankingInputBaseAddress.bytePointer + currentOuterOffset;
                rankingInputAddress.bytePointer =
                    rankingInputOuterAddress.bytePointer + middleOffset;
                rankingIn = rankingInputAddress.wordPointer;

                for (; inner < 5; inner++) {
                    memcpy(rankingOut, rankingIn, 0x10);
                    memcpy(timeDst, timeIn, 0x10);
                    timeDst += 4;
                    timeIn += 4;
                    rankingOut += 4;
                    rankingIn += 4;
                }
                rankingDst += sizeof(g_RankingRecords[0][0]);
                middleOffset += sizeof(g_RankingRecords[0][0]);
            }
            outerDst += sizeof(g_RankingRecords[0]);
            outerOffset += sizeof(g_RankingRecords[0]);
        }
        }

        {
        /* The remaining register hints in these loops are load-bearing. */
        s32 outer = 0;
        register s32 *sectorBase asm("$11") = &g_BestSectorTimes[0][0][0];
        GameSaveBlockAddress outerDestinationAddress;
        u8 *outerDst =
            (outerDestinationAddress.pointer = block,
             outerDestinationAddress.bytePointer);
        s32 outerOffset = 0;

        for (; outer < 2; outer++) {
            s32 currentOuterOffset = (middle = 0, outerOffset);
            GameSaveBlockAddress outerBlockAddress;
            SectorTimeTableAddress sectorDestinationAddress;
            u8 *sectorDst =
                (outerBlockAddress.bytePointer = outerDst,
                 sectorDestinationAddress.pointer =
                     &outerBlockAddress.pointer->bestSectorTimes[0][0][0],
                 sectorDestinationAddress.bytes);
            s32 middleOffset = 0;

            for (; middle < 4; middle++) {
                SectorTimeTableAddress sectorInputBaseAddress;
                SectorTimeTableAddress sectorInputOuterAddress;
                SectorTimeTableAddress sectorInputAddress;
                SectorTimeTableAddress sectorOutputAddress;
                s32 *sectorOut;
                s32 *sectorIn;

                inner = 0;
                sectorOutputAddress.bytes = sectorDst;
                sectorOut = sectorOutputAddress.pointer;
                sectorInputBaseAddress.pointer = sectorBase;
                sectorInputOuterAddress.bytes =
                    sectorInputBaseAddress.bytes + currentOuterOffset;
                sectorInputAddress.bytes =
                    sectorInputOuterAddress.bytes + middleOffset;
                sectorIn = sectorInputAddress.pointer;

                for (; inner < 3; inner++) {
                    *sectorOut = *sectorIn++;
                    sectorOut++;
                }
                sectorDst += sizeof(g_BestSectorTimes[0][0]);
                middleOffset += sizeof(g_BestSectorTimes[0][0]);
            }
            outerDst += sizeof(g_BestSectorTimes[0]);
            outerOffset += sizeof(g_BestSectorTimes[0]);
        }
        }
    }

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
