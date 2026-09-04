#include "game/car.h"
#include "game/course_select_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/render.h"
#include "game/screens.h"
#include "game/state.h"

typedef struct RankingCarSprite {
    s16 x;
    u8 width;
    u8 textureU;
    u8 textureV;
} RankingCarSprite;

enum {
    RANKING_TABLE_DRAW_PHASE_MAX = 12,
    RANKING_TABLE_PROGRESS_MAX = 15,
    RANKING_TABLE_SLIDE_PER_PHASE = 35,
};

static const RankingCarSprite s_carNameSprites[GAME_CAR_COUNT] = {
    {0xAE, 0x14, 0x50, 0xBC}, {0xAE, 0x14, 0x50, 0xBC},
    {0xAE, 0x14, 0x50, 0xBC}, {0xAF, 0x20, 0x00, 0xBC},
    {0xAF, 0x20, 0x64, 0xBC}, {0xAF, 0x20, 0x64, 0xBC},
    {0xAF, 0x20, 0x64, 0xBC}, {0xAF, 0x30, 0x22, 0xBC},
    {0xAF, 0x30, 0x22, 0xBC}, {0xAF, 0x30, 0x22, 0xBC},
    {0xAE, 0x14, 0x50, 0xBC}, {0xAF, 0x20, 0x64, 0xBC},
    {0xAF, 0x30, 0x22, 0xBC},
};

static const RankingCarSprite s_carBadgeSprites[GAME_CAR_COUNT] = {
    {0xE8, 0x2A, 0x16, 0x30}, {0xE8, 0x20, 0x48, 0x30},
    {0xE9, 0x20, 0x7C, 0x30}, {0xE7, 0x34, 0x00, 0x40},
    {0xE9, 0x28, 0x74, 0x50}, {0xE8, 0x2A, 0x3E, 0x50},
    {0xE8, 0x20, 0xB0, 0x50}, {0xE8, 0x28, 0x40, 0x40},
    {0xE9, 0x22, 0x7A, 0x40}, {0xE9, 0x30, 0xA0, 0x40},
    {0xE9, 0x2C, 0xA4, 0x30}, {0xE8, 0x2A, 0x0A, 0x60},
    {0xE8, 0x30, 0x04, 0x50},
};

_Static_assert(sizeof(s_carNameSprites) / sizeof(s_carNameSprites[0]) ==
               GAME_CAR_COUNT,
               "ranking needs a name sprite for every car");
_Static_assert(sizeof(s_carBadgeSprites) / sizeof(s_carBadgeSprites[0]) ==
               GAME_CAR_COUNT,
               "ranking needs a badge sprite for every car");

static void DrawRankingCarSprites(GameOrderingTableEntry *ot, s32 y, s32 carIndex) {
    const RankingCarSprite *name;
    const RankingCarSprite *badge;

    if ((u32)carIndex >= GAME_CAR_COUNT) {
        return;
    }
    name = &s_carNameSprites[carIndex];
    badge = &s_carBadgeSprites[carIndex];
    DrawSprite(ot, name->x, y, name->width, 0x10, name->textureU,
               name->textureV, 0, 0, 0, 0x244, 1, 1, 0x3B);
    DrawSprite(ot, badge->x, y, badge->width, 0x10, badge->textureU,
               badge->textureV, 0, 0, 0, 0x244, 1, 1, 0x3E);
}

static void DrawRankingCourseHeader(GameOrderingTableEntry *ot, s32 slide) {
    CourseLabelSprites label;
    s32 contentY = slide + 0x26C;

    if (!GetCourseLabelSprites(SeriesCourseIndex(), &label)) {
        return;
    }
    DrawSprite(ot, 0xA4, contentY, 0x48, 0x10, 0x48, 0xAC, 0, 0, 0,
               0x244, 1, 1, 0x3B);
    DrawSprite(ot, 0xA4, slide + 0x25C, label.nameWidth, 0x10,
               label.nameTextureU, label.nameTextureV, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    DrawSprite(ot, 0xEC, contentY, 0x20, 0x10,
               label.distanceTextureU, 0xB4, 0, 0, 0, 0x244, 1, 1, 0x3A);
}

/* The animated five-row ranking/time-record panel. */
s32 DrawRankingTable(s32 *progress, s32 step, RankingTableKind table) {
    char text[16];
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    const RaceRecord (*records)[RECORD_TABLE_LENGTH];
    s32 series;
    s32 course;
    s32 phase;
    s32 slide;
    s32 panelY;
    s32 rowY;
    s32 row;
    s32 rowYStep;

    if (progress == NULL) {
        return 0;
    }
    if (step == 0) {
        *progress = 0;
        /* Initialization-only call; its caller ignores the return value. */
        return 0;
    }

    *progress = AddClampedMenuValue(
        *progress, 0, 0, RANKING_TABLE_PROGRESS_MAX);
    if (step < 0) {
        *progress = AddClampedMenuValue(
            *progress, step, 0, RANKING_TABLE_PROGRESS_MAX);
    }

    series = CourseSeries(g_CourseIndex);
    course = SeriesCourseIndex();
    records = table == RANKING_TABLE_LAP ? g_RankingRecords[series]
                                        : g_TimeRecords[series];
    phase = *progress;
    if (phase >= 0) {
        if (phase > RANKING_TABLE_DRAW_PHASE_MAX) {
            phase = RANKING_TABLE_DRAW_PHASE_MAX;
        }
        slide = -phase * RANKING_TABLE_SLIDE_PER_PHASE;
        panelY = slide + 0x21A;

        if (series != 0) {
            DrawSprite(ot, 0xA4, slide + 0x1EA, 0x30, 0x18,
                       0xCC, 0x38, 0, 0, 0, 0x20F, 1, 0, 0x3C);
        }
        DrawSprite(ot, 0xC8, (s16)(slide + 0x218), 0x20, 0x28, 0x48, 0xD8,
                   0, 0, 0, 0x220, 1, 0, 0x19);
        DrawRankingCourseHeader(ot, slide);

        if (table == RANKING_TABLE_LAP) {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x12, 0x10, 0, 0x7C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x2B, rowY, 0x14, 0x10, 0xC0, 0x8C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x41, rowY, 0x2C, 0x10, 0xD4, 0x8C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
        } else {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x1C, 0x10, 0x44, 0x7C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x34, rowY, 0x14, 0x10, 0xC0, 0x8C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x4A, rowY, 0x2C, 0x10, 0xD4, 0x8C, 0, 0, 0,
                       0x244, 1, 1, 0x3B);
        }
        GameDrawMenuButton(0, panelY, 0x99, 0x23, 0, 0, 0);

        for (row = 0, rowYStep = 0x82; row < RECORD_TABLE_LENGTH;
             row++, rowYStep += 0x20) {
            const RaceRecord *record = &records[course][row];

            FormatLapTime(text, record->raceTime);
            rowY = panelY + rowYStep;
            DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F, 0x244,
                          0x20);
            DrawLargeText(0x77, rowY, record->driverName, 0x7F, 0x7F, 0x7F,
                          0x244, 0xA0);

            DrawSprite(ot, 0x17, panelY + rowYStep, 8, 0x10,
                       row * 8 + 8, 0x18, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0xA7, panelY + rowYStep, 8, 0x10, 0x58,
                       0x28, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0xDF, panelY + rowYStep, 8, 0x10, 0x58,
                       0x28, 0, 0, 0, 0x244, 1, 1, 0x3B);

            DrawRankingCarSprites(ot, panelY + rowYStep, record->carIndex);
        }

        DrawLargeText(0x1E, panelY + 0x82, g_MsgOrdinalSt, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, panelY + 0xA2, g_MsgOrdinalNd, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1F, panelY + 0xC2, g_MsgOrdinalRd, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, panelY + 0xE2, g_MsgOrdinalTh, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, panelY + 0x102, g_MsgOrdinalTh, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);

        DrawRectOutline(ot + 1, 0, panelY + 0x7A, 0x124, 0xA0, 0xB4,
                        0xB4, 0xB4, 0xFF);
        DrawSolidRect(ot + 1, 0, panelY + 0x7A, 0x124, 0xA0, 0, 0, 0,
                      0xFF);
    }

    if (step >= 0) {
        *progress = AddClampedMenuValue(
            *progress, step, 0, RANKING_TABLE_PROGRESS_MAX);
        return *progress >= RANKING_TABLE_PROGRESS_MAX;
    }
    return 0;
}
