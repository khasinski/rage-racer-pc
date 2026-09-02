#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum {
    LABELED_RACE_TIME_CAPACITY = LAP_TIME_TEXT_CAPACITY + 2
};

static void FormatLabeledRaceTime(
    char text[LABELED_RACE_TIME_CAPACITY], char label, s32 timeMs) {
    text[0] = label;
    text[1] = '/';
    FormatLapTime(&text[2], timeMs);
}

void DrawRaceTimePanel(s32 slideY) {
    char text[LABELED_RACE_TIME_CAPACITY];
    s32 lapCount = CourseLapCount(g_CourseIndex);
    s32 recordMode = RaceRecordMode(g_GrandPrixMode);
    s32 bestTimeColor;
    s32 lap;

    DrawProportionalText(0x10, slideY + 0x80, g_CaptionTotalTime, 0x7812);

    FormatLabeledRaceTime(text, 'T', g_RaceTotalTime);
    bestTimeColor =
        g_BestTotalTimes[g_GrandPrixSeries][SeriesCourseIndex()]
                        [recordMode] == g_RaceTotalTime
            ? 0x784C
            : 0x7812;
    DrawProportionalText(0x14, slideY + 0x90, text, bestTimeColor);

    DrawProportionalText(0x10, slideY + 0xA4, g_CaptionLapTime, 0x7812);
    for (lap = 0; lap < lapCount; lap++) {
        s32 x = lap < 3 ? 0x14 : 0xB0;
        s32 y = slideY + 0xB0 + (lap % 3) * 0xC;
        s32 color = g_PlayerCar.drive.hudLapHighlightRow == lap
                        ? 0x784C
                        : 0x7812;

        FormatLabeledRaceTime(
            text, (char)('1' + lap),
            g_PlayerCar.lapTimes.table.milliseconds[lap]);
        DrawProportionalText(x, y, text, color);
    }
}
