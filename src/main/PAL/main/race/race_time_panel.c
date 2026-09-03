#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum {
    LABELED_RACE_TIME_CAPACITY = LAP_TIME_TEXT_CAPACITY + 2,
    PANEL_LABEL_X = 0x10,
    PANEL_VALUE_LEFT_X = 0x14,
    PANEL_VALUE_RIGHT_X = 0xB0,
    TOTAL_TIME_LABEL_Y = 0x80,
    TOTAL_TIME_VALUE_Y = 0x90,
    LAP_TIME_LABEL_Y = 0xA4,
    LAP_TIME_VALUE_Y = 0xB0,
    LAP_TIME_ROW_HEIGHT = 0xC,
    LAP_ROWS_PER_COLUMN = 3,
    PANEL_TEXT_CLUT = 0x7812,
    NEW_RECORD_TEXT_CLUT = 0x784C,
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
    s32 series = RaceSeriesIndex(g_GrandPrixSeries);
    s32 bestTimeColor;
    s32 lap;

    DrawProportionalText(PANEL_LABEL_X, slideY + TOTAL_TIME_LABEL_Y,
                         g_CaptionTotalTime, PANEL_TEXT_CLUT);

    FormatLabeledRaceTime(text, 'T', g_RaceTotalTime);
    bestTimeColor =
        g_BestTotalTimes[series][SeriesCourseIndex()]
                        [recordMode] == g_RaceTotalTime
            ? NEW_RECORD_TEXT_CLUT
            : PANEL_TEXT_CLUT;
    DrawProportionalText(PANEL_VALUE_LEFT_X, slideY + TOTAL_TIME_VALUE_Y,
                         text, bestTimeColor);

    DrawProportionalText(PANEL_LABEL_X, slideY + LAP_TIME_LABEL_Y,
                         g_CaptionLapTime, PANEL_TEXT_CLUT);
    for (lap = 0; lap < lapCount; lap++) {
        s32 x = lap < LAP_ROWS_PER_COLUMN ? PANEL_VALUE_LEFT_X
                                          : PANEL_VALUE_RIGHT_X;
        s32 y = slideY + LAP_TIME_VALUE_Y +
                (lap % LAP_ROWS_PER_COLUMN) * LAP_TIME_ROW_HEIGHT;
        s32 color = g_PlayerCar.drive.hudLapHighlightRow == lap
                        ? NEW_RECORD_TEXT_CLUT
                        : PANEL_TEXT_CLUT;

        FormatLabeledRaceTime(
            text, (char)('1' + lap),
            g_PlayerCar.lapTimes.table.milliseconds[lap]);
        DrawProportionalText(x, y, text, color);
    }
}
