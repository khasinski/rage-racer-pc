#include <stdio.h>

#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/records_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"

static void DrawRecordRows(s32 slideX,
                           const RaceRecord records[RECORD_TABLE_LENGTH],
                           s32 insertedRow, char *text, size_t textSize) {
    s32 row;

    for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
        const RaceRecord *record = &records[row];
        s32 carIndex = record->carIndex;
        s32 color = insertedRow == row ? 0x780F : 0x78CC;
        s32 y = 0x78 + row * 0x14;

        text[0] = g_PlaceSuffixNames[row][0];
        text[1] = g_PlaceSuffixNames[row][1];
        text[2] = g_PlaceSuffixNames[row][2];
        text[3] = '/';
        FormatLapTime(&text[4], record->raceTime);
        FormatRecordDriverClass(
            &text[0xC], (s32)(textSize - 0xC), g_FmtRecordName, record,
            g_CarClassNames[carIndex]);
        DrawText8x8(slideX + 0x14, y, text, color);

        snprintf(text, textSize, g_FmtCarName, g_CarNames[carIndex]);
        DrawText8x8(slideX + 0x2C, y + 0xA, text, color);
    }
}

void DrawRankingPanel(s32 slideX) {
    char text[56];
    s32 lapCount;
    s32 row;
    s32 course = SeriesCourseIndex();

    DrawProportionalText(slideX + 0x10, 0x4C, g_CaptionLapTime2, 0x7852);
    text[1] = '/';
    lapCount = CourseLapCount(g_CourseIndex);
    for (row = 0; row < lapCount; row++) {
        s32 column = row % 2;
        s32 x = slideX + 0x14 + (row / 2) * 0x60;
        s32 y = 0x58 + column * 8;
        s32 color = g_BestLapIndex == row ? 0x780F : 0x78CC;

        text[0] = row + '1';
        FormatLapTime(&text[2],
                      g_PlayerCar.lapTimes.table.milliseconds[row]);
        DrawText8x8(x, y, text, color);
    }

    DrawProportionalText(slideX + 0x10, 0x6C, g_CaptionRanking2, 0x7812);
    DrawRecordRows(slideX, g_RankingRecords[g_GrandPrixSeries][course],
                   g_RankingInsertRow, text, sizeof(text));
}

void DrawTimeRecordPanel(s32 slideX) {
    char text[48];
    s32 course = SeriesCourseIndex();

    DrawProportionalText(slideX + 0x10, 0x4C, g_CaptionTotalTime2, 0x7852);

    text[0] = 'T';
    text[1] = '/';
    FormatLapTime(&text[2], g_RaceTotalTime);
    DrawText8x8(slideX + 0x14, 0x58, text, 0x78CC);

    DrawProportionalText(slideX + 0x10, 0x6C, g_CaptionRanking2, 0x7812);
    DrawRecordRows(slideX, g_TimeRecords[g_GrandPrixSeries][course],
                   g_TimeRecordInsertRow, text, sizeof(text));
}

void DrawNameEntryCursor(s32 charIndex, s32 row) {
    if ((g_AnimTimer & 8) == 0) {
        return;
    }

    g_RenderState.packetCursor = AddTilePrim(
        GamePrimaryOrderingTable(0), RENDER_PRIM_CURSOR_AS(u8),
        (charIndex * 8) + 0x7C, ((row * 5) << 2) + 0x7E, 9, 2, 0xC0, 0x48,
        0x48);
}
