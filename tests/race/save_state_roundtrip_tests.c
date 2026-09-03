#include "game/audio.h"
#include "game/input_internal.h"
#include "game/menu.h"
#include "game/prize_money.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/save_internal.h"
#include "game/team_logo.h"

#include <stdio.h>
#include <string.h>

GameRaceProgress g_GrandPrixSave;
GameRaceProgress g_ExtraGrandPrixSave;
GameRaceProgress g_TimeAttackSave;
RaceRecord g_RankingRecords[2][4][5];
RaceRecord g_TimeRecords[2][4][5];

static s32 s_audioApplyCalls;
static s32 s_mappingLoadCalls;
static s32 s_loadedPadMapping;
static s32 s_loadedNegconMapping;
static s32 s_recordRepairCalls;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

void ApplyAudioSettings(void) { s_audioApplyCalls++; }
void LoadPadButtonMapping(s32 padMapping, s32 negconMapping) {
    s_loadedPadMapping = padMapping;
    s_loadedNegconMapping = negconMapping;
    s_mappingLoadCalls++;
}
void RepairRecordTimes(void) { s_recordRepairCalls++; }

static void SetRepresentativeState(void) {
    s32 series;
    s32 course;
    s32 row;

    g_PadMappingIndex = 3;
    g_NegconMappingIndex = 4;
    g_NegconSteerNeutral = 101;
    g_NegconSteerPlay = 2;
    g_NegconNeutralI = 303;
    g_NegconNeutralII = 404;
    g_NegconNeutralL = 505;
    g_NegconMaxTwist = 3;

    g_GrandPrixSave.course = 2;
    g_GrandPrixSave.carIndex = 7;
    g_GrandPrixSave.classIndex = 4;
    g_GrandPrixSave.maxClassReached = 5;
    g_GrandPrixSave.money = 1234567;
    g_ExtraGrandPrixSave.course = 3;
    g_TimeAttackSave.carIndex = 9;
    g_TimeAttackSave.timeAttackSeries = 1;
    g_ExtraGrandPrixUnlocked = 1;
    g_MaxClassReached[0] = 5;
    g_MaxClassReached[1] = 4;

    g_GrandPrixCars[4].modelVariant = 2;
    g_GrandPrixCars[4].tireCompound = 3;
    g_GrandPrixCars[4].transmission = 1;
    g_GrandPrixCars[4].paintColor1 = 6;
    g_GrandPrixCars[4].paintColor2 = 7;
    g_GrandPrixCars[4].enabled = 1;
    g_ClassRecords[2].place = 3;
    g_ClassRecords[2].clears = 12;

    for (series = 0; series < RECORD_SERIES_COUNT; series++) {
        for (course = 0; course < RECORD_COURSE_COUNT; course++) {
            for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
                RaceRecord *ranking = &g_RankingRecords[series][course][row];
                RaceRecord *time = &g_TimeRecords[series][course][row];

                memcpy(ranking->driverName, "DRIVER\0\0", 8);
                ranking->raceTime = 1000 + row;
                ranking->carIndex = row;
                ranking->unused = 0;
                *time = *ranking;
            }
        }
    }

    g_TeamLogoClut[3] = 0x4210;
    g_TeamLogoCanvas.halfwords[17] = 0x1357;
    g_BestLapTimes[1][2][0] = 11111;
    g_BestTotalTimes[0][3][1] = 22222;
    g_BestSectorTimes[1][1][2] = 33333;
    g_RankingRecords[1][2][3].raceTime = 44444;
    g_TimeRecords[0][1][4].carIndex = 11;

    g_BgmSelection = 7;
    g_BgmVolumeSetting = 13;
    g_SfxVolumeSetting = 9;
    g_MonoOutput = 1;
    g_GrandPrixCourseProgress.bestPlace[2] = 3;
    g_ExtraGrandPrixCourseProgress.retriesRemaining = 4;
}

static int ReservedBytesAreZero(const GameSaveBlock *block) {
    s32 group;
    s32 car;

    for (group = 0; group < SAVED_CAR_TABLE_COUNT; group++) {
        for (car = 0; car < GAME_CAR_COUNT; car++) {
            if (block->carSetup[group][car].reserved[0] != 0 ||
                block->carSetup[group][car].reserved[1] != 0) {
                return 0;
            }
        }
    }
    for (car = 0; car < (s32)sizeof(block->reserved); car++) {
        if (block->reserved[car] != 0) return 0;
    }
    return 1;
}

static int SaveBlocksMatch(const GameSaveBlock *actual,
                           const GameSaveBlock *expected) {
    const u8 *actualBytes = (const u8 *)actual;
    const u8 *expectedBytes = (const u8 *)expected;
    size_t offset;

    for (offset = 0; offset < sizeof(*actual); offset++) {
        if (actualBytes[offset] != expectedBytes[offset]) {
            fprintf(stderr,
                    "save blocks differ at 0x%zx: got 0x%02x, expected 0x%02x\n",
                    offset, actualBytes[offset], expectedBytes[offset]);
            return 0;
        }
    }
    return 1;
}

static void ClearSerializedRuntimeState(void) {
    g_PadMappingIndex = 0;
    g_NegconMappingIndex = 0;
    g_NegconSteerNeutral = 0;
    g_NegconSteerPlay = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_NegconMaxTwist = 0;
    memset(&g_GrandPrixSave, 0, sizeof(g_GrandPrixSave));
    memset(&g_ExtraGrandPrixSave, 0, sizeof(g_ExtraGrandPrixSave));
    memset(&g_TimeAttackSave, 0, sizeof(g_TimeAttackSave));
    g_BgmSelection = 0;
    g_ExtraGrandPrixUnlocked = 0;
    memset(g_MaxClassReached, 0, sizeof(g_MaxClassReached));
    memset(g_GrandPrixCars, 0,
           GAME_CAR_COUNT * sizeof(*g_GrandPrixCars));
    memset(g_ExtraGrandPrixCars, 0,
           GAME_CAR_COUNT * sizeof(*g_ExtraGrandPrixCars));
    memset(g_TimeAttackCars, 0,
           GAME_CAR_COUNT * sizeof(*g_TimeAttackCars));
    memset(g_ClassRecords, 0, sizeof(g_ClassRecords));
    memset(g_TeamLogoClut, 0, sizeof(g_TeamLogoClut));
    memset(g_TeamLogoCanvas.bytes, 0, sizeof(g_TeamLogoCanvas.bytes));
    memset(g_BestLapTimes, 0, sizeof(g_BestLapTimes));
    memset(g_BestTotalTimes, 0, sizeof(g_BestTotalTimes));
    memset(g_RankingRecords, 0, sizeof(g_RankingRecords));
    memset(g_TimeRecords, 0, sizeof(g_TimeRecords));
    memset(g_BestSectorTimes, 0, sizeof(g_BestSectorTimes));
    g_BgmVolumeSetting = 0;
    g_SfxVolumeSetting = 0;
    g_MonoOutput = 0;
    memset(&g_GrandPrixCourseProgress, 0,
           sizeof(g_GrandPrixCourseProgress));
    memset(&g_ExtraGrandPrixCourseProgress, 0,
           sizeof(g_ExtraGrandPrixCourseProgress));
}

int main(void) {
    GameSaveBlock saved;
    GameSaveBlock roundTrip;
    GameSaveBlock corrupt;
    GameSaveBlock outOfRange;

    SetRepresentativeState();
    memset(&saved, 0xA5, sizeof(saved));
    StoreSaveStateBlock(&saved);
    CHECK(ReservedBytesAreZero(&saved));
    CHECK(saved.checksum == CalculateSaveBlockChecksum(&saved));

    corrupt = saved;
    corrupt.checksum++;
    g_BgmSelection = 123;
    CHECK(LoadSaveStateBlock(&corrupt) == 0);
    CHECK(g_BgmSelection == 123);
    CHECK(s_audioApplyCalls == 0);
    CHECK(s_mappingLoadCalls == 0);
    CHECK(s_recordRepairCalls == 0);

    ClearSerializedRuntimeState();

    CHECK(LoadSaveStateBlock(&saved) == 1);
    CHECK(s_audioApplyCalls == 1);
    CHECK(s_mappingLoadCalls == 1);
    CHECK(s_loadedPadMapping == g_PadMappingIndex);
    CHECK(s_loadedNegconMapping == g_NegconMappingIndex);
    CHECK(s_recordRepairCalls == 1);
    CHECK(g_GrandPrixSave.money == 1234567);
    CHECK(g_TimeAttackSave.timeAttackSeries == 1);
    CHECK(g_GrandPrixCars[4].paintColor2 == 7);
    CHECK(g_ClassRecords[2].clears == 12);
    CHECK(g_TeamLogoCanvas.halfwords[17] == 0x1357);

    memset(&roundTrip, 0x5A, sizeof(roundTrip));
    StoreSaveStateBlock(&roundTrip);
    CHECK(SaveBlocksMatch(&roundTrip, &saved));

    outOfRange = saved;
    outOfRange.padMappingIndex = 0xFF;
    outOfRange.negconMappingIndex = 0xFE;
    outOfRange.negconSteerPlay = 0xFF;
    outOfRange.negconMaxTwist = 0xFF;
    outOfRange.grandPrixProgress.course = 99;
    outOfRange.grandPrixProgress.carIndex = -20;
    outOfRange.grandPrixProgress.classIndex = 99;
    outOfRange.grandPrixProgress.maxClassReached = -20;
    outOfRange.grandPrixProgress.money = -1;
    outOfRange.extraGrandPrixProgress.money = RACE_MAX_PRIZE_MONEY + 1;
    outOfRange.timeAttackProgress.timeAttackSeries = 99;
    outOfRange.bgmSelection = 99;
    outOfRange.extraGrandPrixUnlocked = 7;
    outOfRange.maxClassReached[0] = 99;
    outOfRange.maxClassReached[1] = -20;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].modelVariant = 0xFF;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].tireCompound = 0xFF;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].transmission = 7;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].paintColor1 = 0xFF;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].paintColor2 = 0xFF;
    outOfRange.carSetup[SAVED_CARS_GRAND_PRIX][0].enabled = 7;
    memset(outOfRange.rankingRecords[0][0][0].driverName, 0xFF,
           sizeof(outOfRange.rankingRecords[0][0][0].driverName));
    outOfRange.rankingRecords[0][0][0].carIndex = -1;
    memset(outOfRange.timeRecords[0][0][0].driverName, 'A',
           sizeof(outOfRange.timeRecords[0][0][0].driverName));
    outOfRange.timeRecords[0][0][0].carIndex = 100;
    outOfRange.classRecords[0].grade = 0xFFFF;
    outOfRange.classRecords[0].clears = 0xFFFF;
    memset(outOfRange.grandPrixCourseProgress, 0xFF,
           sizeof(outOfRange.grandPrixCourseProgress));
    outOfRange.checksum = CalculateSaveBlockChecksum(&outOfRange);
    CHECK(LoadSaveStateBlock(&outOfRange) == 1);
    CHECK(g_PadMappingIndex == CONTROLLER_MAPPING_LAST);
    CHECK(g_NegconMappingIndex == CONTROLLER_MAPPING_LAST);
    CHECK(g_NegconSteerPlay == NEGCON_CALIBRATION_LAST);
    CHECK(g_NegconMaxTwist == NEGCON_CALIBRATION_LAST);
    CHECK(s_loadedPadMapping == CONTROLLER_MAPPING_LAST);
    CHECK(s_loadedNegconMapping == CONTROLLER_MAPPING_LAST);
    CHECK(g_GrandPrixSave.course == COURSE_LONG_SLOT);
    CHECK(g_GrandPrixSave.carIndex == 0);
    CHECK(g_GrandPrixSave.classIndex == GRAND_PRIX_FINAL_CLASS_INDEX);
    CHECK(g_GrandPrixSave.maxClassReached == -1);
    CHECK(g_GrandPrixSave.money == 0);
    CHECK(g_ExtraGrandPrixSave.money == RACE_MAX_PRIZE_MONEY);
    CHECK(g_TimeAttackSave.timeAttackSeries == RECORD_SERIES_COUNT - 1);
    CHECK(g_BgmSelection == 10);
    CHECK(g_ExtraGrandPrixUnlocked == 1);
    CHECK(g_MaxClassReached[0] == GRAND_PRIX_FINAL_CLASS_INDEX);
    CHECK(g_MaxClassReached[1] == 0);
    CHECK(g_GrandPrixCars[0].modelVariant == 0);
    CHECK(g_GrandPrixCars[0].tireCompound == CAR_TIRE_COMPOUND_COUNT - 1);
    CHECK(g_GrandPrixCars[0].transmission == 1);
    CHECK(g_GrandPrixCars[0].paintColor1 == MENU_PAINT_COLOR_COUNT - 1);
    CHECK(g_GrandPrixCars[0].paintColor2 == MENU_PAINT_COLOR_COUNT - 1);
    CHECK(g_GrandPrixCars[0].enabled == 1);
    CHECK(memcmp(g_RankingRecords[0][0][0].driverName, "??????\0\0", 8) ==
          0);
    CHECK(g_RankingRecords[0][0][0].carIndex == 0);
    CHECK(memcmp(g_TimeRecords[0][0][0].driverName, "AAAAAA\0\0", 8) == 0);
    CHECK(g_TimeRecords[0][0][0].carIndex == GAME_CAR_COUNT - 1);
    CHECK(g_ClassRecords[0].place == -1);
    CHECK(g_ClassRecords[0].clears == 99);
    CHECK(g_GrandPrixCourseProgress.bestPlace[0] == 0xFF);
    CHECK(g_GrandPrixCourseProgress.unlockPending == 1);
    CHECK(g_GrandPrixCourseProgress.retriesRemaining == 0);

    outOfRange.bgmSelection = -1;
    outOfRange.checksum = CalculateSaveBlockChecksum(&outOfRange);
    CHECK(LoadSaveStateBlock(&outOfRange) == 1);
    CHECK(g_BgmSelection == 0);

    puts("save state blocks are deterministic and survive a full round trip");
    return 0;
}
