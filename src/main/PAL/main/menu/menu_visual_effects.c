#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/screens.h"
#include "game/state.h"

void RestoreTeamLogoClut(void) { LoadImage(&g_TeamLogoClutRect, &g_TeamLogoBlankClut); }

void UploadTeamLogoClut(void) { LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut); }

typedef union PackedCoordinate {
    s32 value;
    struct {
        u16 fraction;
        u16 integer;
    } parts;
} PackedCoordinate;

/* GCC 2.6.3 needs the concrete outer bound for the indexed view below. */

/* The animated five-row ranking/time-record panel. */
s32 DrawRankingTable(s32 *progress, s32 step, s32 ranking) {
    char text[16];
    OT_TYPE *ot;
    s32 phase;
    u32 slide;
    s32 headerTextureU;
    s16 panelY;
    s16 contentY;
    s16 rowY;
    s32 row;
    s16 rowYStep;
    s32 loopClut;
    s32 spriteHeight;
    s32 spriteOne;
    PackedCoordinate badgeX;
    s16 car;
    s32 badgeXWord;
    s16 rectLeft;

    ot = RENDER_OT_BASE_AS(OT_TYPE);
    if (step == 0) {
        *progress = 0;
        /* Initialization-only call; its caller ignores the return value. */
        return 0;
    }

    if (step < 0) {
        s32 currentValue;
        s32 value;

        currentValue = *progress;
        value = step + currentValue;
        *progress = value;
        if (value < 0) {
            *progress = 0;
        }
    }

    phase = *progress;
    if (phase >= 0) {
        u32 slidePhase;

        if (phase >= 13) {
            phase = 12;
        }
        slidePhase = phase;
        slide = (slidePhase * -1120) >> 5;
        panelY = slide + 0x21A;

        if (g_CourseIndex >= 4) {
            DrawSprite(ot, 0xA4, (s16)(slide + 0x1EA), 0x30, 0x18,
                             0xCC, 0x38, 0, 0, 0, 0x20F, 1, 0, 0x3C);
        }
        DrawSprite(ot, 0xC8, (s16)(slide + 0x218), 0x20, 0x28,
                         0x48, 0xD8, 0, 0, 0, 0x220, 1, 0, 0x19);

        {
            s32 headerClut;
            s32 headerFlags;
            s32 contentYWide;

            headerClut = 0x244;
            headerFlags = 0x3B;
            headerTextureU = 0xA4;
            contentYWide = slide + 0x26C;
            contentY = contentYWide;
            DrawSprite(ot, 0xA4, contentY, 0x48, 0x10, 0x48, 0xAC,
                             0, 0, 0, headerClut, 1, 1, headerFlags);
            switch (SeriesCourseIndex()) {
            case 0:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x54, 0x10,
                                 0, 0x9C, 0, 0, 0, headerClut, 1, 1, headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x44, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 1:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x4C, 0x10,
                                 0x54, 0x9C, 0, 0, 0, headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x64, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 2:
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x48, 0x10,
                                 0, 0xAC, 0, 0, 0, headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10, 0x84, 0xB4,
                                 0, 0, 0, headerClut, 1, 1, 0x3A);
                break;
            case 3: {
                DrawSprite(ot, 0xA4, (s16)(slide + 0x25C), 0x5C, 0x10,
                                 headerTextureU, 0x9C, 0, 0, 0,
                                 headerClut, 1, 1,
                                 headerFlags);
                DrawSprite(ot, 0xEC, contentY, 0x20, 0x10,
                                 headerTextureU, 0xB4, 0, 0, 0,
                                 headerClut, 1, 1, 0x3A);
                break;
            }
            }
        }

        if (ranking != 0) {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x12, 0x10, 0, 0x7C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x2B, rowY, 0x14, 0x10, 0xC0, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x41, rowY, 0x2C, 0x10, 0xD4, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
        } else {
            rowY = panelY + 0xA;
            DrawSprite(ot, 0x18, rowY, 0x1C, 0x10, 0x44, 0x7C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x34, rowY, 0x14, 0x10, 0xC0, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x4A, rowY, 0x2C, 0x10, 0xD4, 0x8C,
                             0, 0, 0, 0x244, 1, 1, 0x3B);
        }
        GameDrawMenuButton(0, panelY, 0x99, 0x23, 0, 0, 0);

        rectLeft = 0;
        loopClut = 0x244;
        spriteHeight = 0x10;
        spriteOne = 1;
        badgeX.value = 0;
        row = 0;
        badgeX.parts.integer = 0xE8;
        rowYStep = 0x82;
        do {
            if (ranking != 0) {
                FormatLapTime(
                    text,
                    g_RankingRecords[CourseSeries(g_CourseIndex)]
                                    [(SeriesCourseIndex())][row].raceTime);
                rowY = panelY + rowYStep;
                DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F,
                                  loopClut, 0x20);
                DrawLargeText(
                    0x77, rowY,
                    g_RankingRecords[CourseSeries(g_CourseIndex)]
                                    [(SeriesCourseIndex())][row].driverName,
                    0x7F, 0x7F, 0x7F, loopClut, 0xA0);
            } else {
                FormatLapTime(
                    text,
                    g_TimeRecords[CourseSeries(g_CourseIndex)]
                                 [(SeriesCourseIndex())][row].raceTime);
                rowY = panelY + rowYStep;
                DrawLargeText(0x36, rowY, text, 0x7F, 0x7F, 0x7F,
                                  loopClut, 0x20);
                DrawLargeText(
                    0x77, rowY,
                    g_TimeRecords[CourseSeries(g_CourseIndex)]
                                 [(SeriesCourseIndex())][row].driverName,
                    0x7F, 0x7F, 0x7F, loopClut, 0xA0);
            }

            DrawSprite(ot, 0x17, (s16)(panelY + rowYStep), 8,
                             spriteHeight, (s16)(row * 8 + 8), 0x18,
                             0, 0, 0, loopClut, spriteOne, spriteOne, 0x3B);
            DrawSprite(ot, 0xA7, (s16)(panelY + rowYStep), 8,
                             spriteHeight, 0x58, 0x28, 0, 0, 0, loopClut,
                             spriteOne, spriteOne, 0x3B);
            DrawSprite(ot, 0xDF, (s16)(panelY + rowYStep), 8,
                             spriteHeight, 0x58, 0x28, 0, 0, 0, loopClut,
                             spriteOne, spriteOne, 0x3B);

            if (ranking != 0) {
                car = (u16)g_RankingRecords[CourseSeries(g_CourseIndex)]
                                             [(SeriesCourseIndex())][row].carIndex;
            } else {
                car = (u16)g_TimeRecords[CourseSeries(g_CourseIndex)]
                                          [(SeriesCourseIndex())][row].carIndex;
            }
            switch (car) {
                case 0:
                case 1:
                case 2:
                case 10:
                    DrawSprite(ot, 0xAE, (s16)(panelY + rowYStep),
                                     0x14, spriteHeight, 0x50,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 3:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 4:
                case 5:
                case 6:
                case 11:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x64,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
                case 7:
                case 8:
                case 9:
                case 12:
                    DrawSprite(ot, 0xAF, (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0x22,
                                     0xBC, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3B);
                    break;
            }

            switch (car) {
                case 0:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x16,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 1:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x48,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 2:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0x7C,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 10:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x2C, spriteHeight, 0xA4,
                                     0x30, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 3:
                    DrawSprite(ot, 0xE7, (s16)(panelY + rowYStep),
                                     0x34, spriteHeight, 0,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 4:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x28, spriteHeight, 0x74,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 5:
                    DrawSprite(ot, badgeX.value >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x3E,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 6:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x20, spriteHeight, 0xB0,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 11:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x2A, spriteHeight, 0x0A,
                                     0x60, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 7:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x28, spriteHeight, 0x40,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 8:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x22, spriteHeight, 0x7A,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 9:
                    DrawSprite(ot, 0xE9, (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0xA0,
                                     0x40, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
                case 12:
                    badgeXWord = badgeX.value;
                    DrawSprite(ot, badgeXWord >> 16,
                                     (s16)(panelY + rowYStep),
                                     0x30, spriteHeight, 0x04,
                                     0x50, 0, 0, 0, loopClut,
                                     spriteOne, spriteOne, 0x3E);
                    break;
            }

            rowYStep += 0x20;
            row++;
        } while (row < 5);

        {
            s32 suffixX;
            s32 textShade;
            s32 textClut;
            s32 textFlags;
            const char *lastSuffix;

            suffixX = 0x1E;
            
            textShade = 0x7F;
            textClut = 0x244;
            textFlags = 0x20;
            DrawLargeText(suffixX, (s16)(panelY + 0x82),
                              g_MsgOrdinalSt, textShade, textShade,
                              textShade, textClut, textFlags);
            DrawLargeText(suffixX, (s16)(panelY + 0xA2),
                              g_MsgOrdinalNd, textShade, textShade,
                              textShade, textClut, textFlags);
            DrawLargeText(0x1F, (s16)(panelY + 0xC2),
                              g_MsgOrdinalRd, textShade, textShade,
                              textShade, textClut, textFlags);
            lastSuffix = g_MsgOrdinalTh;
            DrawLargeText(suffixX, (s16)(panelY + 0xE2), lastSuffix,
                              textShade, textShade, textShade, textClut,
                              textFlags);
            DrawLargeText(suffixX, (s16)(panelY + 0x102), lastSuffix,
                              textShade, textShade, textShade, textClut,
                              textFlags);
        }

        {
            u32 *rectOt;
            s32 rectX;
            s16 rectY;
            s32 rectHeight;
            s32 rectAlpha;

            rectOt = (u32 *)(ot + 1);
            rectX = rectLeft;
            rectY = panelY + 0x7A;
            rectHeight = 0xA0;
            rectAlpha = 0xFF;
            DrawRectOutline(rectOt, rectX, rectY, 0x124,
                                rectHeight, 0xB4, 0xB4, 0xB4, rectAlpha);
            DrawSolidRect(rectOt, rectX, rectY, 0x124, rectHeight,
                              0, 0, 0, rectAlpha);
        }
    }

    if (step >= 0) {
        s32 nextProgress;

        nextProgress = step + *progress;
        if (nextProgress >= 15) {
            *progress = 15;
            return 1;
        }
        *progress = nextProgress;
    }
    return 0;
}
