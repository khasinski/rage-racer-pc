#include "common.h"
#include "game/screens.h"
#include "game/race.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/records_internal.h"

void InitRecordTables(void) {
    s32 series;
    s32 course;
    s32 slot;
    const s32 *defaultLapTimes = g_DefaultLapTimes;
    const s32 *defaultTotalTimes = g_DefaultTotalTimes;

    for (series = 0; series < 2; series++) {
        for (course = 0; course < 4; course++) {
            for (slot = 0; slot < 2; slot++) {
                g_BestLapTimes[series][course][slot] = defaultLapTimes[series * 4 + course];
                g_BestTotalTimes[series][course][slot] = defaultTotalTimes[series * 4 + course];
            }
            for (slot = 0; slot < 5; slot++) {
                g_RankingRecords[series][course][slot] = (RaceRecord){0};
                g_TimeRecords[series][course][slot] = (RaceRecord){0};
            }
        }
    }
}

void *FormatLapTime(void *dst, s32 value) {
    s32 minutes = value / 60000;
    s32 ticks = value / 1000;
    s32 seconds = ticks - (minutes * 60);
    s32 fraction = value - (ticks * 1000);

    sprintf(dst, g_FmtLapTime, minutes, seconds, fraction);
    return dst;
}

void DrawCourseIntro(void) {
    DrawProportionalText(0x10, 0x1C, g_TextTimeAttack, 0x7812);
    DrawText8x8Trans(0x10, 0x39, g_TextCourseIn, 0x78CC);
    DrawResultScreen();
}
