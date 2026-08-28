#include "common.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/memcard.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "psyq/gpu.h"
#include "game/input_internal.h"
#include "game/save_internal.h"

s32 LoadSaveStateBlock(GameSaveBlock *block) {
    register GameSaveBlock *base asm("$17") = block;
    s32 i;
    __asm__("" : "=r"(base) : "0"(base));
    {
        u32 sum;
        u32 checksumIndex;
        GameSaveBlockAddress checksumAddress;
        u16 *p;

        i = 0;
        __asm__("" : "=r"(i) : "0"(i));
        sum = i;
        checksumAddress.pointer = base;
        p = checksumAddress.halfwordPointer;
        do {
            sum += *p++;
            i++;
            checksumIndex = i;
        } while (checksumIndex < 0x7FE);
        printf("%s", g_MsgSaveChecksumOk);
        sum = ~sum;
        printf(g_FmtSaveChecksum, base->checksum, sum);
        if (base->checksum != sum) {
            return 0;
        }
    }

    {
        GameSaveBlockAddress padMappingAddress;
        GameSaveBlockAddress negconMappingAddress;
        GameSaveBlockAddress steerNeutralAddress;
        GameSaveBlockAddress steerPlayAddress;
        GameSaveBlockAddress neutralIIAddress;
        GameSaveBlockAddress neutralLAddress;
        u16 padMappingIndex;
        u16 negconMappingIndex;
        u16 negconSteerNeutral;
        u16 negconSteerPlay;

        padMappingAddress.halfwordPointer = &base->padMappingIndex;
        padMappingIndex = *padMappingAddress.halfwordPointer;
        negconMappingAddress.halfwordPointer = &base->negconMappingIndex;
        negconMappingIndex = *negconMappingAddress.halfwordPointer;
        steerNeutralAddress.halfwordPointer = &base->negconSteerNeutral;
        negconSteerNeutral = *steerNeutralAddress.halfwordPointer;
        steerPlayAddress.halfwordPointer = &base->negconSteerPlay;
        negconSteerPlay = *steerPlayAddress.halfwordPointer;
        g_NegconNeutralI = base->negconNeutralI;
        neutralIIAddress.halfwordPointer = &base->negconNeutralII;
        g_NegconNeutralII = *neutralIIAddress.halfwordPointer;
        neutralLAddress.halfwordPointer = &base->negconNeutralL;
        g_NegconNeutralL = *neutralLAddress.halfwordPointer;
        {
            GameSaveBlockAddress maxTwistAddress;
            u16 hE;
            s32 extraMaxClass;

            maxTwistAddress.halfwordPointer = &base->negconMaxTwist;
            hE = *maxTwistAddress.halfwordPointer;
            g_GrandPrixSave.course = base->grandPrixProgress.course;
            g_GrandPrixSave.carIndex = base->grandPrixProgress.carIndex;
            g_GrandPrixSave.classIndex = base->grandPrixProgress.classIndex;
            g_GrandPrixSave.maxClassReached = base->grandPrixProgress.maxClassReached;
            g_GrandPrixSave.money.value =
                base->grandPrixProgress.money;
            g_ExtraGrandPrixSave.course = base->extraGrandPrixProgress.course;
            g_ExtraGrandPrixSave.carIndex = base->extraGrandPrixProgress.carIndex;
            g_ExtraGrandPrixSave.classIndex = base->extraGrandPrixProgress.classIndex;
            extraMaxClass = base->extraGrandPrixProgress.maxClassReached;
            g_PadMappingIndex = padMappingIndex;
            g_NegconMappingIndex = negconMappingIndex;
            g_NegconSteerNeutral = negconSteerNeutral;
            g_NegconSteerPlay = negconSteerPlay;
            g_NegconMaxTwist = hE;
            g_ExtraGrandPrixSave.maxClassReached = extraMaxClass;
        }
        g_ExtraGrandPrixSave.money.value =
            base->extraGrandPrixProgress.money;
        g_TimeAttackSave.course = base->timeAttackProgress.course;
        g_TimeAttackSave.carIndex = base->timeAttackProgress.carIndex;
        g_TimeAttackSave.classIndex = base->timeAttackProgress.classIndex;
        g_TimeAttackSave.maxClassReached = base->timeAttackProgress.maxClassReached;
        g_TimeAttackSave.money.value =
            base->timeAttackProgress.money;
        {
            s32 bgmSelection = base->bgmSelection;
            u16 extraGrandPrixUnlocked = base->extraGrandPrixUnlocked;
            s32 maxClassReached1;
            g_MaxClassReached[0] = base->maxClassReached[0];
            maxClassReached1 = base->maxClassReached[1];
            g_BgmSelection = bgmSelection;
            g_ExtraGrandPrixUnlocked = extraGrandPrixUnlocked;
            g_MaxClassReached[1] = maxClassReached1;
        }
    }

    {
        s32 i;

        for (i = 0; i < 13; i++) {
            SavedCarSetup *grandPrixCar;
            SavedCarSetup *extraGrandPrixCar;
            SavedCarSetup *timeAttackCar;

            grandPrixCar = &base->carSetup[0][i];
            extraGrandPrixCar = &base->carSetup[1][i];
            timeAttackCar = &base->carSetup[2][i];

            g_GrandPrixCars[i].modelVariant = grandPrixCar->modelVariant;
            g_GrandPrixCars[i].tireCompound = grandPrixCar->tireCompound;
            g_GrandPrixCars[i].transmission = grandPrixCar->transmission;
            g_GrandPrixCars[i].paintColor1 = grandPrixCar->paintColor1;
            g_GrandPrixCars[i].paintColor2 = grandPrixCar->paintColor2;
            g_GrandPrixCars[i].enabled = grandPrixCar->enabled;
            g_ExtraGrandPrixCars[i].modelVariant = extraGrandPrixCar->modelVariant;
            g_ExtraGrandPrixCars[i].tireCompound = extraGrandPrixCar->tireCompound;
            g_ExtraGrandPrixCars[i].transmission = extraGrandPrixCar->transmission;
            g_ExtraGrandPrixCars[i].paintColor1 = extraGrandPrixCar->paintColor1;
            g_ExtraGrandPrixCars[i].paintColor2 = extraGrandPrixCar->paintColor2;
            g_ExtraGrandPrixCars[i].enabled = extraGrandPrixCar->enabled;
            g_TimeAttackCars[i].modelVariant = timeAttackCar->modelVariant;
            g_TimeAttackCars[i].tireCompound = timeAttackCar->tireCompound;
            g_TimeAttackCars[i].transmission = timeAttackCar->transmission;
            g_TimeAttackCars[i].paintColor1 = timeAttackCar->paintColor1;
            g_TimeAttackCars[i].paintColor2 = timeAttackCar->paintColor2;
            g_TimeAttackCars[i].enabled = timeAttackCar->enabled;
        }
    }

    {
        s32 index = 0;

        for (; index < 11; index++) {
            SavedClassRecord *saved = &base->classRecords[index];

            g_ClassRecords[index].place = saved->grade;
            g_ClassRecords[index].clears = saved->clears;
        }
    }

    memcpy(g_TeamLogoClut, base->teamLogoClut, sizeof(base->teamLogoClut));
    memcpy(g_TeamLogoCanvas.halfwords, base->teamLogoCanvas,
           sizeof(base->teamLogoCanvas));
    memcpy(g_BestLapTimes, base->bestLapTimes, sizeof(g_BestLapTimes));
    memcpy(g_BestTotalTimes, base->bestTotalTimes,
           sizeof(g_BestTotalTimes));
    memcpy(g_RankingRecords, base->rankingRecords,
           sizeof(base->rankingRecords));
    memcpy(g_TimeRecords, base->timeRecords, sizeof(base->timeRecords));
    memcpy(g_BestSectorTimes, base->bestSectorTimes,
           sizeof(g_BestSectorTimes));
    RepairRecordTimes();

    /* g_BgmVolumeSetting / g_SfxVolumeSetting / g_MonoOutput clamps */
    {
        s32 v = base->bgmVolume;
        s32 c;
        g_BgmVolumeSetting = v;
        if (v >= 0) {
            c = v;
            if (c >= 0x10) {
                c = 0xF;
            }
        } else {
            c = 0;
        }
        v = base->sfxVolume;
        g_BgmVolumeSetting = c;
        g_SfxVolumeSetting = v;
        if (v >= 0) {
            c = v;
            if (c >= 0x10) {
                c = 0xF;
            }
        } else {
            c = 0;
        }
        v = base->monoOutput;
        g_SfxVolumeSetting = c;
        g_MonoOutput = v;
        if (v != 0) {
            g_MonoOutput = 1;
        }
    }

    /* g_GrandPrixCourseProgress / g_ExtraGrandPrixCourseProgress unaligned copies */
    memcpy(&g_GrandPrixCourseProgress, base->grandPrixCourseProgress, 8);
    memcpy(&g_ExtraGrandPrixCourseProgress, base->extraGrandPrixCourseProgress, 8);

    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
    ApplyAudioSettings();
    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    return 1;
}
