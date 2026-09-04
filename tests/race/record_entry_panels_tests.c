#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/records_internal.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

#include <stdio.h>
#include <string.h>

RaceRecord g_RankingRecords[2][4][RECORD_TABLE_LENGTH];
RaceRecord g_TimeRecords[2][4][RECORD_TABLE_LENGTH];
s32 g_BestLapIndex;
s32 g_CourseIndex;
s16 g_GrandPrixSeries;
s32 g_RaceTotalTime;
s32 g_AnimTimer;
s32 g_RankingInsertRow;
s32 g_TimeRecordInsertRow;
PlayerCarRuntime g_PlayerCar;
GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;

char g_CaptionLapTime2[] = "LAPS";
char g_CaptionRanking2[] = "RANKING";
char g_CaptionTotalTime2[] = "TOTAL";
char g_FmtRecordName[] = "/%s/%s";
char g_FmtCarName[] = "/%s";
const char *g_NativeCarNames[GAME_CAR_COUNT] = {"CAR0", "CAR1", "CAR2"};
const char *g_NativeCarClassNames[GAME_CAR_COUNT] = {"C0", "C1", "C2"};
static const char s_first[] = "1ST";
static const char s_second[] = "2ND";
static const char s_third[] = "3RD";
static const char s_fourth[] = "4TH";
static const char s_fifth[] = "5TH";
const char *const g_PlaceSuffixNames[RECORD_TABLE_LENGTH] = {
    s_first, s_second, s_third, s_fourth, s_fifth,
};

typedef struct TextCall {
    s32 proportional;
    s32 x;
    s32 y;
    s32 color;
    char text[64];
} TextCall;

static TextCall s_calls[16];
static s32 s_callCount;
static s32 s_tileCalls;

void FormatLapTime(char dst[LAP_TIME_TEXT_CAPACITY], s32 timeMs) {
    snprintf(dst, LAP_TIME_TEXT_CAPACITY, "0'%02d\"000", timeMs / 1000);
}

static void RecordText(s32 proportional, s32 x, s32 y, const char *text,
                       s32 color) {
    TextCall *call = &s_calls[s_callCount++];

    call->proportional = proportional;
    call->x = x;
    call->y = y;
    call->color = color;
    snprintf(call->text, sizeof(call->text), "%s", text);
}

void DrawText8x8(s32 x, s32 y, const char *text, s32 color) {
    RecordText(0, x, y, text, color);
}

void DrawProportionalText(s32 x, s32 y, const char *text, s32 color) {
    RecordText(1, x, y, text, color);
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                s32 width, s32 height, s32 red, s32 green, s32 blue) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    s_tileCalls++;
    return packet;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void SeedRecords(RaceRecord records[RECORD_TABLE_LENGTH]) {
    s32 row;

    for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
        snprintf(records[row].driverName, sizeof(records[row].driverName),
                 "N%d", row);
        records[row].raceTime = (row + 1) * 1000;
        records[row].carIndex = (s16)(row % 3);
    }
}

int main(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_CourseIndex = 1;
    SeedRecords(g_RankingRecords[0][1]);
    SeedRecords(g_TimeRecords[0][1]);
    g_PlayerCar.lapTimes.table.milliseconds[0] = 1000;
    g_PlayerCar.lapTimes.table.milliseconds[1] = 2000;
    g_PlayerCar.lapTimes.table.milliseconds[2] = 3000;
    g_BestLapIndex = 1;

    DrawRankingPanel(10);
    CHECK(s_callCount == 15);
    CHECK(s_calls[0].proportional && s_calls[0].x == 26 &&
          strcmp(s_calls[0].text, "LAPS") == 0);
    CHECK(strcmp(s_calls[1].text, "1/0'01\"000") == 0);
    CHECK(s_calls[2].x == 30 && s_calls[2].y == 0x60);
    CHECK(s_calls[2].color == 0x780F);
    CHECK(s_calls[3].x == 126 && s_calls[3].y == 0x58);
    if (strcmp(s_calls[5].text, "1ST/0'01\"000/N0/C0") != 0) {
        fprintf(stderr, "unexpected record row: '%s'\n", s_calls[5].text);
        return 1;
    }
    CHECK(strcmp(s_calls[6].text, "/CAR0") == 0);
    CHECK(s_calls[5].color == 0x780F && s_calls[6].color == 0x780F);
    CHECK(s_calls[7].color == 0x78CC);

    s_callCount = 0;
    g_RaceTotalTime = 9000;
    DrawTimeRecordPanel(-4);
    CHECK(s_callCount == 13);
    CHECK(strcmp(s_calls[0].text, "TOTAL") == 0);
    CHECK(strcmp(s_calls[1].text, "T/0'09\"000") == 0);
    CHECK(strcmp(s_calls[3].text, "1ST/0'01\"000/N0/C0") == 0);
    CHECK(s_calls[3].x == 16 && s_calls[3].y == 0x78);

    s_callCount = 0;
    g_GrandPrixSeries = -1;
    DrawRankingPanel(0);
    DrawTimeRecordPanel(0);
    CHECK(s_callCount == 0);

    g_GrandPrixSeries = 0;
    memset(g_RankingRecords[0][1][0].driverName, 'X', 8);
    g_RankingRecords[0][1][0].carIndex = -1;
    s_callCount = 0;
    DrawRankingPanel(0);
    CHECK(strstr(s_calls[5].text, "XXXXXXXX/C0") != NULL);
    CHECK(strcmp(s_calls[6].text, "/CAR0") == 0);

    g_AnimTimer = 8;
    s_tileCalls = 0;
    DrawNameEntryCursor(0, 0);
    CHECK(s_tileCalls == 1);
    DrawNameEntryCursor(-1, 0);
    DrawNameEntryCursor(0, RECORD_TABLE_LENGTH);
    CHECK(s_tileCalls == 1);

    puts("record entry panel tests passed");
    return 0;
}
