#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

#include <stdio.h>
#include <string.h>

s32 g_BestTotalTimes[2][4][2];
char g_CaptionLapTime[] = "LAP TIME";
char g_CaptionTotalTime[] = "TOTAL TIME";
s32 g_CourseIndex;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
PlayerCarRuntime g_PlayerCar;
s32 g_RaceTotalTime;

typedef struct TextCall {
    s32 x, y, color;
    char text[24];
} TextCall;

static TextCall s_calls[9];
static s32 s_callCount;

/* Spelled as the header spells it: gcc holds a definition to the
 * declaration's array bound. */
void FormatLapTime(char dst[LAP_TIME_TEXT_CAPACITY], s32 timeMs) {
    snprintf(dst, 22, "%d", timeMs);
}

void DrawProportionalText(s32 x, s32 y, const char *text, s32 color) {
    TextCall *call = &s_calls[s_callCount++];
    call->x = x;
    call->y = y;
    call->color = color;
    snprintf(call->text, sizeof(call->text), "%s", text);
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(g_BestTotalTimes, 0, sizeof(g_BestTotalTimes));
    g_CourseIndex = 1;
    g_GrandPrixMode = 0;
    g_GrandPrixSeries = 0;
    g_RaceTotalTime = 1234;
    g_PlayerCar.drive.hudLapHighlightRow = 1;
    g_PlayerCar.lapTimes.table.milliseconds[0] = 101;
    g_PlayerCar.lapTimes.table.milliseconds[1] = 202;
    g_PlayerCar.lapTimes.table.milliseconds[2] = 303;
    s_callCount = 0;
}

int main(void) {
    Reset();
    g_BestTotalTimes[0][1][0] = g_RaceTotalTime;
    DrawRaceTimePanel(5);
    CHECK(s_callCount == 6);
    CHECK(s_calls[0].x == 0x10 && s_calls[0].y == 0x85);
    CHECK(strcmp(s_calls[1].text, "T/1234") == 0);
    CHECK(s_calls[1].color == 0x784C);
    CHECK(strcmp(s_calls[3].text, "1/101") == 0);
    CHECK(strcmp(s_calls[4].text, "2/202") == 0);
    CHECK(s_calls[3].color == 0x7812 && s_calls[4].color == 0x784C);
    CHECK(s_calls[5].y == 5 + 0xB0 + 2 * 0xC);

    Reset();
    g_CourseIndex = 7;
    g_GrandPrixSeries = 1;
    g_PlayerCar.drive.hudLapHighlightRow = 5;
    g_PlayerCar.lapTimes.table.milliseconds[3] = 404;
    g_PlayerCar.lapTimes.table.milliseconds[4] = 505;
    g_PlayerCar.lapTimes.table.milliseconds[5] = 606;
    DrawRaceTimePanel(0);
    CHECK(s_callCount == 9);
    CHECK(s_calls[1].color == 0x7812);
    CHECK(s_calls[6].x == 0xB0 && s_calls[6].y == 0xB0);
    CHECK(strcmp(s_calls[8].text, "6/606") == 0);
    CHECK(s_calls[8].x == 0xB0 && s_calls[8].color == 0x784C);

    Reset();
    g_GrandPrixMode = 2;
    g_BestTotalTimes[0][1][1] = g_RaceTotalTime;
    DrawRaceTimePanel(0);
    CHECK(s_calls[1].color == 0x784C);

    puts("race time panel tests passed");
    return 0;
}
