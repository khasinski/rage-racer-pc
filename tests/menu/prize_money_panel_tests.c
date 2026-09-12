#include "common.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

typedef struct TextRecord {
    s32 x;
    s32 y;
    s32 clut;
    char text[16];
} TextRecord;

s32 g_ClassPromoted;
GameRaceProgress *g_RaceProgress;

static GameRaceProgress s_progress;
static TextRecord s_records[6];
static s32 s_recordCount;

void DrawProportionalText(s32 x, s32 y, const char *text, s32 clut) {
    TextRecord *record = &s_records[s_recordCount++];

    record->x = x;
    record->y = y;
    record->clut = clut;
    snprintf(record->text, sizeof(record->text), "%s", text);
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_RaceProgress = &s_progress;
    s_progress.money = 67890;
    g_ClassPromoted = 0;

    DrawPrizeMoneyPanel(20, 12345, 0);
    CHECK(s_recordCount == 4);
    CHECK(s_records[0].x == 0x10 && s_records[0].y == 148 &&
          strcmp(s_records[0].text, "hci") == 0);
    CHECK(s_records[1].x == 0x12 && s_records[1].y == 160 &&
          strcmp(s_records[1].text, "12345v") == 0);
    CHECK(s_records[2].y == 180 && strcmp(s_records[2].text, "hebi") == 0);
    CHECK(s_records[3].y == 192 && strcmp(s_records[3].text, "67890v") == 0);
    CHECK(s_records[3].clut == 0x7812);

    s_recordCount = 0;
    g_ClassPromoted = 1;
    DrawPrizeMoneyPanel(0, 0, 500000);
    CHECK(s_recordCount == 6);
    CHECK(s_records[4].y == 192 && strcmp(s_records[4].text, "hji") == 0);
    CHECK(s_records[5].y == 204 && strcmp(s_records[5].text, "500000v") == 0);

    s_recordCount = 0;
    g_RaceProgress = NULL;
    g_ClassPromoted = 0;
    DrawPrizeMoneyPanel(0, 0, 0);
    CHECK(s_recordCount == 4);
    CHECK(strcmp(s_records[3].text, "0v") == 0);

    puts("prize money panel tests passed");
    return 0;
}
