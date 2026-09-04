#include "common.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>

s32 g_CourseIndex;
RaceRecord g_RankingRecords[2][4][5];
RaceRecord g_TimeRecords[2][4][5];
const char g_MsgOrdinalSt[4] = "ST";
const char g_MsgOrdinalNd[4] = "ND";
const char g_MsgOrdinalRd[4] = "RD";
const char g_MsgOrdinalTh[8] = "TH";
GameRenderState g_RenderState;

typedef struct CarSpriteRecord {
    s32 x;
    s32 width;
    s32 u;
    s32 v;
    s32 flags;
} CarSpriteRecord;

static CarSpriteRecord s_carSprites[32];
static s32 s_carSpriteCount;
static s32 s_spriteCount;
static s32 s_firstSpriteY;
static s32 s_textCount;
static const char *s_firstDriverName;
static s32 s_buttonY;
static s32 s_panelRectY;
static CarSpriteRecord s_courseHeader;
static s32 s_courseBadgeU;

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 u,
                u16 v, u8 r, u8 g, u8 b, u16 clut, s32 shade,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shade;
    (void)semiTrans;
    if (s_spriteCount == 0) {
        s_firstSpriteY = y;
    }
    s_spriteCount++;
    if (flags == 0x3E ||
        (flags == 0x3B && (x == 0xAE || x == 0xAF))) {
        s_carSprites[s_carSpriteCount++] =
            (CarSpriteRecord){x, width, u, v, (s32)flags};
    }
    if (x == 0xA4 && flags == 0x3B && !(u == 0x48 && v == 0xAC)) {
        s_courseHeader = (CarSpriteRecord){x, width, u, v, (s32)flags};
    }
    if (x == 0xEC && flags == 0x3A) {
        s_courseBadgeU = u;
    }
}

void DrawLargeText(s32 x, s16 y, const char *text, u8 r, u8 g, u8 b,
                   u16 clut, s32 flags) {
    (void)x;
    (void)y;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    if (flags == 0xA0 && s_firstDriverName == NULL) {
        s_firstDriverName = text;
    }
    s_textCount++;
}

void FormatLapTime(char *destination, s32 time) {
    char *text = destination;

    snprintf(text, 16, "%d", time);
}

void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height, u8 r, u8 g,
                        u8 b) {
    (void)x;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_buttonY = y;
}

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height, u8 r,
                     u8 g, u8 b, u8 alpha) {
    (void)ot;
    (void)x;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
    s_panelRectY = y;
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetDraws(void) {
    s_carSpriteCount = 0;
    s_spriteCount = 0;
    s_textCount = 0;
    s_firstDriverName = NULL;
    s_buttonY = -1;
    s_panelRectY = -1;
    s_courseHeader.width = 0;
    s_courseBadgeU = -1;
}

static int CheckCarSprites(s32 firstCar, s32 carCount) {
    static const CarSpriteRecord names[13] = {
        {0xAE, 0x14, 0x50, 0xBC, 0x3B}, {0xAE, 0x14, 0x50, 0xBC, 0x3B},
        {0xAE, 0x14, 0x50, 0xBC, 0x3B}, {0xAF, 0x20, 0x00, 0xBC, 0x3B},
        {0xAF, 0x20, 0x64, 0xBC, 0x3B}, {0xAF, 0x20, 0x64, 0xBC, 0x3B},
        {0xAF, 0x20, 0x64, 0xBC, 0x3B}, {0xAF, 0x30, 0x22, 0xBC, 0x3B},
        {0xAF, 0x30, 0x22, 0xBC, 0x3B}, {0xAF, 0x30, 0x22, 0xBC, 0x3B},
        {0xAE, 0x14, 0x50, 0xBC, 0x3B}, {0xAF, 0x20, 0x64, 0xBC, 0x3B},
        {0xAF, 0x30, 0x22, 0xBC, 0x3B}};
    static const CarSpriteRecord badges[13] = {
        {0xE8, 0x2A, 0x16, 0x30, 0x3E}, {0xE8, 0x20, 0x48, 0x30, 0x3E},
        {0xE9, 0x20, 0x7C, 0x30, 0x3E}, {0xE7, 0x34, 0x00, 0x40, 0x3E},
        {0xE9, 0x28, 0x74, 0x50, 0x3E}, {0xE8, 0x2A, 0x3E, 0x50, 0x3E},
        {0xE8, 0x20, 0xB0, 0x50, 0x3E}, {0xE8, 0x28, 0x40, 0x40, 0x3E},
        {0xE9, 0x22, 0x7A, 0x40, 0x3E}, {0xE9, 0x30, 0xA0, 0x40, 0x3E},
        {0xE9, 0x2C, 0xA4, 0x30, 0x3E}, {0xE8, 0x2A, 0x0A, 0x60, 0x3E},
        {0xE8, 0x30, 0x04, 0x50, 0x3E}};
    s32 i;

    CHECK(s_carSpriteCount == carCount * 2);
    for (i = 0; i < carCount; i++) {
        CHECK(memcmp(&s_carSprites[i * 2], &names[firstCar + i],
                     sizeof(CarSpriteRecord)) == 0);
        CHECK(memcmp(&s_carSprites[i * 2 + 1], &badges[firstCar + i],
                     sizeof(CarSpriteRecord)) == 0);
    }
    return 1;
}

int main(void) {
    static const s32 headerWidth[4] = {0x54, 0x4C, 0x48, 0x5C};
    static const s32 headerU[4] = {0x00, 0x54, 0x00, 0xA4};
    static const s32 headerV[4] = {0x9C, 0x9C, 0xAC, 0x9C};
    static const s32 badgeU[4] = {0x44, 0x64, 0x84, 0xA4};
    GameOrderingTableEntry orderingTable[4];
    s32 progress;
    s32 batch;
    s32 row;

    RENDER_OT_BASE = orderingTable;
    strcpy(g_RankingRecords[0][0][0].driverName, "RANK");
    strcpy(g_TimeRecords[0][0][0].driverName, "TIME");
    strcpy(g_TimeRecords[1][0][0].driverName, "EXTRA");

    progress = 8;
    DrawRankingTable(&progress, 0, 0);
    CHECK(progress == 0 && s_spriteCount == 0);

    for (batch = 0; batch < 3; batch++) {
        s32 firstCar = batch * 5;
        s32 count = firstCar < 10 ? 5 : 3;

        for (row = 0; row < 5; row++) {
            g_TimeRecords[0][0][row].carIndex =
                (s16)(row < count ? firstCar + row : 99);
        }
        ResetDraws();
        progress = 12;
        CHECK(DrawRankingTable(&progress, 1, 0) == 0);
        CHECK(progress == 13);
        CHECK(CheckCarSprites(firstCar, count));
        CHECK(s_buttonY == 0x76 && s_panelRectY == 0xF0);
        CHECK(s_firstSpriteY == 0x74 && s_textCount == 15);
        CHECK(s_firstDriverName == g_TimeRecords[0][0][0].driverName);
    }

    for (g_CourseIndex = 0; g_CourseIndex < 4; g_CourseIndex++) {
        ResetDraws();
        progress = 12;
        DrawRankingTable(&progress, 1, 0);
        CHECK(s_courseHeader.width == headerWidth[g_CourseIndex]);
        CHECK(s_courseHeader.u == headerU[g_CourseIndex]);
        CHECK(s_courseHeader.v == headerV[g_CourseIndex]);
        CHECK(s_courseBadgeU == badgeU[g_CourseIndex]);
    }
    g_CourseIndex = 0;

    ResetDraws();
    progress = 15;
    CHECK(DrawRankingTable(&progress, 1, 1) == 1 && progress == 15);
    CHECK(s_firstDriverName == g_RankingRecords[0][0][0].driverName);

    ResetDraws();
    g_CourseIndex = 4;
    progress = 15;
    CHECK(DrawRankingTable(&progress, 1, 0) == 1);
    CHECK(s_firstDriverName == g_TimeRecords[1][0][0].driverName);

    ResetDraws();
    g_CourseIndex = 0;
    progress = 1;
    DrawRankingTable(&progress, -4, 0);
    CHECK(progress == 0 && s_buttonY == 0x21A);

    ResetDraws();
    progress = INT_MAX;
    CHECK(DrawRankingTable(&progress, 1, 0) == 1 && progress == 15);

    ResetDraws();
    progress = INT_MIN;
    CHECK(DrawRankingTable(&progress, -1, 0) == 0 && progress == 0);
    CHECK(s_buttonY == 0x21A);

    ResetDraws();
    progress = INT_MIN;
    CHECK(DrawRankingTable(&progress, 1, 0) == 0 && progress == 1);
    CHECK(s_buttonY == 0x21A);

    ResetDraws();
    progress = 1;
    CHECK(DrawRankingTable(&progress, INT_MAX, 0) == 1 && progress == 15);

    CHECK(DrawRankingTable(NULL, 1, 0) == 0);

    puts("ranking table preserves all car sprites, records and transitions");
    return 0;
}
