#include "game/audio.h"
#include "game/input_internal.h"
#include "game/menu.h"
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
    g_PadMappingIndex = 3;
    g_NegconMappingIndex = 4;
    g_NegconSteerNeutral = 101;
    g_NegconSteerPlay = 202;
    g_NegconNeutralI = 303;
    g_NegconNeutralII = 404;
    g_NegconNeutralL = 505;
    g_NegconMaxTwist = 606;

    g_GrandPrixSave.course = 2;
    g_GrandPrixSave.carIndex = 7;
    g_GrandPrixSave.classIndex = 4;
    g_GrandPrixSave.maxClassReached = 5;
    g_GrandPrixSave.money.value = 1234567;
    g_ExtraGrandPrixSave.course = 3;
    g_TimeAttackSave.carIndex = 9;
    g_ExtraGrandPrixUnlocked = 1;
    g_MaxClassReached[0] = 6;
    g_MaxClassReached[1] = 8;

    g_GrandPrixCars[4].modelVariant = 2;
    g_GrandPrixCars[4].tireCompound = 3;
    g_GrandPrixCars[4].transmission = 1;
    g_GrandPrixCars[4].paintColor1 = 6;
    g_GrandPrixCars[4].paintColor2 = 7;
    g_GrandPrixCars[4].enabled = 1;
    g_ClassRecords[2].place = 4;
    g_ClassRecords[2].clears = 12;

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

    for (group = 0; group < 3; group++) {
        for (car = 0; car < 13; car++) {
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

int main(void) {
    GameSaveBlock saved;
    GameSaveBlock roundTrip;
    GameSaveBlock corrupt;

    SetRepresentativeState();
    memset(&saved, 0xA5, sizeof(saved));
    StoreSaveStateBlock(&saved);
    CHECK(ReservedBytesAreZero(&saved));
    CHECK(saved.checksum == CalculateSaveBlockChecksum(&saved));

    corrupt = saved;
    corrupt.checksum++;
    CHECK(LoadSaveStateBlock(&corrupt) == 0);
    CHECK(s_audioApplyCalls == 0);
    CHECK(s_mappingLoadCalls == 0);
    CHECK(s_recordRepairCalls == 0);

    memset(g_GrandPrixCars, 0, 13 * sizeof(*g_GrandPrixCars));
    memset(g_ClassRecords, 0, sizeof(g_ClassRecords));
    memset(g_TeamLogoCanvas.bytes, 0, sizeof(g_TeamLogoCanvas.bytes));
    g_GrandPrixSave.money.value = 0;
    g_BgmSelection = 0;
    g_BgmVolumeSetting = 0;
    g_SfxVolumeSetting = 0;
    g_MonoOutput = 0;

    CHECK(LoadSaveStateBlock(&saved) == 1);
    CHECK(s_audioApplyCalls == 1);
    CHECK(s_mappingLoadCalls == 1);
    CHECK(s_loadedPadMapping == g_PadMappingIndex);
    CHECK(s_loadedNegconMapping == g_NegconMappingIndex);
    CHECK(s_recordRepairCalls == 1);
    CHECK(g_GrandPrixSave.money.value == 1234567);
    CHECK(g_GrandPrixCars[4].paintColor2 == 7);
    CHECK(g_ClassRecords[2].clears == 12);
    CHECK(g_TeamLogoCanvas.halfwords[17] == 0x1357);

    memset(&roundTrip, 0x5A, sizeof(roundTrip));
    StoreSaveStateBlock(&roundTrip);
    CHECK(memcmp(&roundTrip, &saved, sizeof(saved)) == 0);

    puts("save state blocks are deterministic and survive a full round trip");
    return 0;
}
