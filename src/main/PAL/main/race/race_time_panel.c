#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum { SPECIAL_COURSE_INDEX = 3 };

void DrawRaceTimePanel(s32 slideY) {
    char text[24];
    s32 lapCount = g_CourseIndex == SPECIAL_COURSE_INDEX ? 6 : 3;
    s32 bestTimeColor;
    s32 lap;

    DrawProportionalText(0x10, slideY + 0x80, g_CaptionTotalTime, 0x7812);

    text[0] = 'T';
    text[1] = '/';
    FormatLapTime(&text[2], g_RaceTotalTime);
    bestTimeColor =
        g_BestTotalTimes[g_GrandPrixSeries][SeriesCourseIndex()]
                        [g_GrandPrixMode] == g_RaceTotalTime
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

        text[0] = (char)('1' + lap);
        FormatLapTime(&text[2], g_PlayerCar.lapTimes.table.milliseconds[lap]);
        DrawProportionalText(x, y, text, color);
    }
}
