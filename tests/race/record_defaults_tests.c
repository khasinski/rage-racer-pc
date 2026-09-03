#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "game/menu_types.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include "game/state.h"

#define CHECK(condition) do { if (!(condition)) abort(); } while (0)

s32 g_DefaultLapTimes[8] = {
    100765, 146765, 145765, 35765, 97765, 135765, 128765, 35765,
};
s32 g_DefaultTotalTimes[8] = {
    310765, 448765, 445765, 220765, 301765, 415765, 394765, 220765,
};
s32 g_BestLapTimes[2][4][2];
s32 g_BestTotalTimes[2][4][2];
s32 g_BestSectorTimes[2][4][3];
RaceRecord g_RankingRecords[2][4][5];
RaceRecord g_TimeRecords[2][4][5];

char g_FmtLapTime[] = "%d'%02d\"%03d";
int main(void) {
    static const char expectedNames[5][8] = {
        {'R', 'A', 'G', 'E', ' ', ' ', '\0', '\0'},
        {'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'},
        {'N', 'A', 'M', 'C', 'O', ' ', '\0', '\0'},
        {'R', 'I', 'D', 'G', 'E', ' ', '\0', '\0'},
        {'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'},
    };
    static const s16 expectedCars[5] = {0, 3, 4, 7, 3};
    s32 series;
    s32 course;
    s32 slot;

    {
        const s32 threeLaps[] = {91000, 89999, 90000};
        const s32 sixLaps[] = {81000, 80000, 79000, 78000, 77000, 76000};
        const s32 tiedLaps[] = {70000, 70000, 71000};
        const s32 longLaps[] = {700000, 650000, 800000};
        FastestLap fastest;

        fastest = FindFastestLap(threeLaps, 3);
        CHECK(fastest.index == 1 && fastest.time == 89999);
        fastest = FindFastestLap(sixLaps, 6);
        CHECK(fastest.index == 5 && fastest.time == 76000);
        fastest = FindFastestLap(tiedLaps, 3);
        CHECK(fastest.index == 0 && fastest.time == 70000);
        fastest = FindFastestLap(longLaps, 3);
        CHECK(fastest.index == 1 && fastest.time == 650000);
        fastest = FindFastestLap(longLaps, 0);
        CHECK(fastest.index == -1 && fastest.time == 0);
        fastest = FindFastestLap(NULL, -1);
        CHECK(fastest.index == -1 && fastest.time == 0);
        fastest = FindFastestLap(NULL, 3);
        CHECK(fastest.index == -1 && fastest.time == 0);
    }

    memset(g_RankingRecords, 0xA5, sizeof(g_RankingRecords));
    memset(g_TimeRecords, 0xA5, sizeof(g_TimeRecords));
    InitRecordTables();

    {
        struct {
            char formatted[LAP_TIME_TEXT_CAPACITY];
            char guard;
        } output = {{0}, '!'};

        FormatLapTime(output.formatted, 0);
        CHECK(strcmp(output.formatted, "0'00\"000") == 0);
        FormatLapTime(output.formatted, 125678);
        CHECK(strcmp(output.formatted, "2'05\"678") == 0);
        FormatLapTime(output.formatted, 2147483647);
        CHECK(strcmp(output.formatted, "9'59\"999") == 0);
        CHECK(output.guard == '!');
        FormatLapTime(output.formatted, -1);
        CHECK(strcmp(output.formatted, "0'00\"000") == 0);
        FormatLapTime(NULL, 1234);
    }

    for (series = 0; series < 2; series++) {
        for (course = 0; course < 4; course++) {
            s32 index = series * 4 + course;
            for (slot = 0; slot < 2; slot++) {
                CHECK(g_BestLapTimes[series][course][slot] ==
                      g_DefaultLapTimes[index]);
                CHECK(g_BestTotalTimes[series][course][slot] ==
                      g_DefaultTotalTimes[index]);
            }
            for (slot = 0; slot < 3; slot++) {
                CHECK(g_BestSectorTimes[series][course][slot] ==
                      g_DefaultLapTimes[index]);
            }
            for (slot = 0; slot < 5; slot++) {
                RaceRecord *ranking = &g_RankingRecords[series][course][slot];
                RaceRecord *time = &g_TimeRecords[series][course][slot];
                CHECK(memcmp(ranking->driverName, expectedNames[slot], 8) == 0);
                CHECK(memcmp(time->driverName, expectedNames[slot], 8) == 0);
                CHECK(ranking->raceTime ==
                      g_DefaultLapTimes[index] + slot * 2000);
                CHECK(time->raceTime ==
                      g_DefaultTotalTimes[index] + slot * 10000);
                CHECK(ranking->carIndex == expectedCars[slot]);
                CHECK(time->carIndex == expectedCars[slot]);
            }
        }
    }

    g_BestLapTimes[1][2][0] = 0;
    g_BestLapTimes[1][2][1] = 12345;
    g_BestTotalTimes[0][3][0] = -1;
    g_BestTotalTimes[0][3][1] = 23456;
    g_BestSectorTimes[1][1][0] = 0;
    g_BestSectorTimes[1][1][1] = -1;
    g_BestSectorTimes[1][1][2] = 34567;
    RepairRecordTimes();
    CHECK(g_BestLapTimes[1][2][0] == g_DefaultLapTimes[6]);
    CHECK(g_BestLapTimes[1][2][1] == 12345);
    CHECK(g_BestTotalTimes[0][3][0] == g_DefaultTotalTimes[3]);
    CHECK(g_BestTotalTimes[0][3][1] == 23456);
    CHECK(g_BestSectorTimes[1][1][0] == g_DefaultLapTimes[5]);
    CHECK(g_BestSectorTimes[1][1][1] == g_DefaultLapTimes[5]);
    CHECK(g_BestSectorTimes[1][1][2] == 34567);

    {
        u8 nameCodes[6] = {0};
        RaceRecord *records = g_RankingRecords[0][0];
        CHECK(InsertRaceRecord(records, 103000, 11, nameCodes) == 2);
        CHECK(records[0].raceTime == 100765);
        CHECK(records[1].raceTime == 102765);
        CHECK(records[2].raceTime == 103000);
        CHECK(records[3].raceTime == 104765);
        CHECK(records[4].raceTime == 106765);
        CHECK(records[2].carIndex == 11);
        CHECK(memcmp(records[2].driverName, "AAAAAA\0\0", 8) == 0);
        for (slot = 0; slot < 6; slot++) CHECK(nameCodes[slot] == 0xB);

        memset(nameCodes, 0, sizeof(nameCodes));
        CHECK(InsertRaceRecord(records, 100000, 12, nameCodes) == 0);
        CHECK(records[0].raceTime == 100000 && records[0].carIndex == 12);
        CHECK(records[1].raceTime == 100765);
        CHECK(records[4].raceTime == 104765);

        memset(nameCodes, 0, sizeof(nameCodes));
        CHECK(InsertRaceRecord(records, 100765, 13, nameCodes) == 2);
        CHECK(records[1].raceTime == 100765);
        CHECK(records[2].raceTime == 100765 && records[2].carIndex == 13);

        memset(nameCodes, 0x55, sizeof(nameCodes));
        CHECK(InsertRaceRecord(records, 999999, 12, nameCodes) == 5);
        for (slot = 0; slot < 6; slot++) CHECK(nameCodes[slot] == 0x55);
    }

    return 0;
}
