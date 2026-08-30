#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "game/menu_types.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/save_internal.h"
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
char g_TextTimeAttack[] = "TIME ATTACK";
char g_TextCourseIn[] = "COURSE IN";

void DrawProportionalText(s32 x, s32 y, char *text, s32 color) {
    (void)x;
    (void)y;
    (void)text;
    (void)color;
}

void DrawText8x8Trans(s32 x, s32 y, char *text, s32 color) {
    (void)x;
    (void)y;
    (void)text;
    (void)color;
}

void DrawResultScreen(void) {}

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

    memset(g_RankingRecords, 0xA5, sizeof(g_RankingRecords));
    memset(g_TimeRecords, 0xA5, sizeof(g_TimeRecords));
    InitRecordTables();

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
        CHECK(InsertRaceRecord(records, 999999, 12, nameCodes) == 5);
    }

    return 0;
}
