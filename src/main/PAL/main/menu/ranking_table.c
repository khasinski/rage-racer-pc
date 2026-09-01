#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/screens.h"
#include "game/state.h"

typedef struct RankingCarSprite {
    s16 x;
    u8 width;
    u8 textureU;
    u8 textureV;
} RankingCarSprite;

typedef struct RankingCourseHeader {
    u8 width;
    u8 titleU;
    u8 titleV;
    u8 badgeU;
} RankingCourseHeader;

static const RankingCourseHeader s_courseHeaders[4] = {
    {0x54, 0x00, 0x9C, 0x44},
    {0x4C, 0x54, 0x9C, 0x64},
    {0x48, 0x00, 0xAC, 0x84},
    {0x5C, 0xA4, 0x9C, 0xA4},
};

static const RankingCarSprite s_carNameSprites[13] = {
    {0xAE, 0x14, 0x50, 0xBC}, {0xAE, 0x14, 0x50, 0xBC},
    {0xAE, 0x14, 0x50, 0xBC}, {0xAF, 0x20, 0x00, 0xBC},
    {0xAF, 0x20, 0x64, 0xBC}, {0xAF, 0x20, 0x64, 0xBC},
    {0xAF, 0x20, 0x64, 0xBC}, {0xAF, 0x30, 0x22, 0xBC},
    {0xAF, 0x30, 0x22, 0xBC}, {0xAF, 0x30, 0x22, 0xBC},
    {0xAE, 0x14, 0x50, 0xBC}, {0xAF, 0x20, 0x64, 0xBC},
    {0xAF, 0x30, 0x22, 0xBC},
};

static const RankingCarSprite s_carBadgeSprites[13] = {
    {0xE8, 0x2A, 0x16, 0x30}, {0xE8, 0x20, 0x48, 0x30},
    {0xE9, 0x20, 0x7C, 0x30}, {0xE7, 0x34, 0x00, 0x40},
    {0xE9, 0x28, 0x74, 0x50}, {0xE8, 0x2A, 0x3E, 0x50},
    {0xE8, 0x20, 0xB0, 0x50}, {0xE8, 0x28, 0x40, 0x40},
    {0xE9, 0x22, 0x7A, 0x40}, {0xE9, 0x30, 0xA0, 0x40},
    {0xE9, 0x2C, 0xA4, 0x30}, {0xE8, 0x2A, 0x0A, 0x60},
    {0xE8, 0x30, 0x04, 0x50},
};

static void DrawRankingCarSprites(OT_TYPE *ot, s32 y, s32 carIndex) {
    const RankingCarSprite *name;
    const RankingCarSprite *badge;

    if ((u32)carIndex >= 13) {
        return;
    }
    name = &s_carNameSprites[carIndex];
    badge = &s_carBadgeSprites[carIndex];
    DrawSprite(ot, name->x, (s16)y, name->width, 0x10, name->textureU,
               name->textureV, 0, 0, 0, 0x244, 1, 1, 0x3B);
    DrawSprite(ot, badge->x, (s16)y, badge->width, 0x10, badge->textureU,
               badge->textureV, 0, 0, 0, 0x244, 1, 1, 0x3E);
}

static void DrawRankingCourseHeader(OT_TYPE *ot, s32 slide) {
    const RankingCourseHeader *header = &s_courseHeaders[SeriesCourseIndex()];
    s32 contentY = slide + 0x26C;

    DrawSprite(ot, 0xA4, (s16)contentY, 0x48, 0x10, 0x48, 0xAC, 0, 0, 0,
               0x244, 1, 1, 0x3B);
    DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), header->width, 0x10,
               header->titleU, header->titleV, 0, 0, 0, 0x244, 1, 1, 0x3B);
    DrawSprite(ot, 0xEC, (s16)contentY, 0x20, 0x10, header->badgeU, 0xB4, 0,
               0, 0, 0x244, 1, 1, 0x3A);
}

/* The animated five-row ranking/time-record panel. */
s32 DrawRankingTable(s32 *progress, s32 step, s32 ranking) {
    char text[16];
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE);
    s32 phase;
    s32 slide;
    s16 panelY;
    s16 rowY;
    s32 row;
    s16 rowYStep;

    if (step == 0) {
        *progress = 0;
        /* Initialization-only call; its caller ignores the return value. */
        return 0;
    }

    if (step < 0) {
        *progress += step;
        if (*progress < 0) {
            *progress = 0;
        }
    }

    phase = *progress;
    if (phase >= 0) {
        if (phase > 12) {
            phase = 12;
        }
        slide = -phase * 35;
        panelY = slide + 0x21A;

        if (g_CourseIndex >= 4) {
            DrawSprite(ot, 0xA4, (s16)(slide + 0x1EA), 0x30, 0x18,
                       0xCC, 0x38, 0, 0, 0, 0x20F, 1, 0, 0x3C);
        }
        DrawSprite(ot, 0xC8, (s16)(slide + 0x218), 0x20, 0x28, 0x48, 0xD8,
                   0, 0, 0, 0x220, 1, 0, 0x19);
        DrawRankingCourseHeader(ot, slide);

        if (ranking != 0) {
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

        for (row = 0, rowYStep = 0x82; row < 5;
             row++, rowYStep += 0x20) {
            const RaceRecord *record;

            if (ranking != 0) {
                record = &g_RankingRecords[CourseSeries(g_CourseIndex)]
                                          [SeriesCourseIndex()][row];
            } else {
                record = &g_TimeRecords[CourseSeries(g_CourseIndex)]
                                       [SeriesCourseIndex()][row];
            }
            FormatLapTime(text, record->raceTime);
            rowY = panelY + rowYStep;
            DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F, 0x244,
                          0x20);
            DrawLargeText(0x77, rowY, record->driverName, 0x7F, 0x7F, 0x7F,
                          0x244, 0xA0);

            DrawSprite(ot, 0x17, (s16)(panelY + rowYStep), 8, 0x10,
                       (s16)(row * 8 + 8), 0x18, 0, 0, 0, 0x244, 1, 1,
                       0x3B);
            DrawSprite(ot, 0xA7, (s16)(panelY + rowYStep), 8, 0x10, 0x58,
                       0x28, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0xDF, (s16)(panelY + rowYStep), 8, 0x10, 0x58,
                       0x28, 0, 0, 0, 0x244, 1, 1, 0x3B);

            DrawRankingCarSprites(ot, panelY + rowYStep, record->carIndex);
        }

        DrawLargeText(0x1E, (s16)(panelY + 0x82), g_MsgOrdinalSt, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, (s16)(panelY + 0xA2), g_MsgOrdinalNd, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1F, (s16)(panelY + 0xC2), g_MsgOrdinalRd, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, (s16)(panelY + 0xE2), g_MsgOrdinalTh, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);
        DrawLargeText(0x1E, (s16)(panelY + 0x102), g_MsgOrdinalTh, 0x7F,
                      0x7F, 0x7F, 0x244, 0x20);

        DrawRectOutline(ot + 1, 0, panelY + 0x7A, 0x124, 0xA0, 0xB4,
                        0xB4, 0xB4, 0xFF);
        DrawSolidRect(ot + 1, 0, panelY + 0x7A, 0x124, 0xA0, 0, 0, 0,
                      0xFF);
    }

    if (step >= 0) {
        *progress += step;
        if (*progress >= 15) {
            *progress = 15;
            return 1;
        }
    }
    return 0;
}
