#include "game/course_select_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"

/* Draws the first half of a sliding label and returns its Y for the rest. */
static s32 DrawSlidingSprite(
    void *ot,
    s32 x,
    s32 baseY,
    s32 slide,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 r,
    s32 g,
    s32 b,
    s32 clut,
    s32 shadeTex,
    s32 semiTrans,
    s32 flags) {
    s32 y;

    y = baseY - slide;
    DrawSprite(
        ot, x, y, w, h, u, v, r, g, b, clut,
        shadeTex, semiTrans, flags);
    return y;
}

s32 DrawCourseSelectScreen(s32 step) {
    GameOrderingTableEntry *ot;
    u8 fade;
    s32 slide;
    CourseClassHeaderSprite classHeader;
    s32 coordinateY;
    s32 lineColor;
    s32 row;
    s32 digitCount;
    s32 prizeOffset;
    s32 course;
    CourseLabelSprites courseLabel;
    CourseSelectScrollFrame scroll;
    if (step == 0) {
        g_CourseSelectScrollProgress = 0;
        return 0;
    }

    scroll = AdvanceCourseSelectScroll(g_CourseSelectScrollProgress, step);
    g_CourseSelectScrollProgress = scroll.progress;
    slide = scroll.slide;

    if (g_MenuAltLayout != 0) {
        return g_CourseSelectScrollProgress;
    }

    /* The reset and alternate-layout paths do not draw.  Resolve the
     * ordering-table layer only once we know this frame needs it, so those
     * paths remain valid before the renderer has installed an OT. */
    ot = RENDER_OT_BASE + 1;
    fade = (u8)(g_CourseSelectScrollProgress / 4);
    course = SeriesCourseIndex();

    if (g_GrandPrixMode != 0) {
        if (GetCourseClassHeaderSprite(
                g_SeriesSelection, g_GrandPrixClass, &classHeader)) {
            DrawSprite(ot, 0x50, 0xB0 - slide, classHeader.width, 0x10,
                       classHeader.textureU, classHeader.textureV, fade, fade,
                       fade, 0x244, 0, 1, 0x5B);
            DrawSprite(
                ot, classHeader.width + 0x50, 0xB0 - slide, 0x10,
                0x10, 0xEC, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
        }

        coordinateY = DrawSlidingSprite(
            ot, 0x50, 0x97, slide, 0x1A, 0x10, 0x60, 0xCC,
            fade, fade, fade, 0x244, 0, 1, 0x5B);
        if (CourseSelectClassIndexValid(g_GrandPrixClass)) {
            DrawSprite(
                ot, 0x6C, coordinateY, 8, 0x10,
                g_GrandPrixClass * 8 + 8, 0x18,
                fade, fade, fade, 0x244, 0, 1, 0x5B);
        }

        lineColor = fade * 2;
        DrawLine(
            ot, 0x48, 0xAA - slide, 0xAF, 0xAA - slide,
            lineColor, lineColor, lineColor, 0x40);
        DrawLine(
            ot, 0x48, 0xAB - slide, 0xAF, 0xAB - slide,
            lineColor, lineColor, lineColor, 0x40);
        DrawSolidRect(
            ot, 0x48, 0x94 - slide, 0x68, 0x30,
            lineColor, lineColor, lineColor,
            fade < 0x7F ? 0x20 : 0xFF);
        DrawSprite(
            ot, 0xB0, 0x94 - slide, 0x20, 0x30,
            0x60, 0x88, fade, fade, fade, 0x25B, 0, 1, 0x39);
    }

    coordinateY = DrawSlidingSprite(
        ot, 0x4C, 0xD0, slide, 0x18, 0xC, 0x18, 0xDC,
        fade, fade, fade, 0x244, 0, 1, 0x3A);
    DrawSprite(
        ot, 0x68, coordinateY, 0x12, 0xC,
        0x32, 0xDC, fade, fade, fade, 0x244, 0, 1, 0x3A);

    coordinateY = DrawSlidingSprite(
        ot, 0x4C, 0xF8, slide, 0x18, 0xC, 0x18, 0xDC,
        fade, fade, fade, 0x244, 0, 1, 0x3A);
    DrawSprite(
        ot, 0x68, coordinateY, 0x1A, 0xC,
        0x46, 0xDC, fade, fade, fade, 0x244, 0, 1, 0x3A);

    if (GetCourseLabelSprites(course, &courseLabel)) {
        coordinateY = DrawSlidingSprite(
            ot, 0x4C, 0xE0, slide, 8, 0x10, courseLabel.prefixTextureU, 0x18,
            fade, fade, fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x54, coordinateY, courseLabel.nameWidth, 0x10,
            courseLabel.nameTextureU, courseLabel.nameTextureV, fade, fade,
            fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x4C, 0x108 - slide, courseLabel.distanceWidth, 0x10,
            courseLabel.distanceTextureU, 0xB4, fade, fade, fade, 0x244, 0, 1,
            0x3A);
    }

    if (g_GrandPrixMode != 0) {
        DrawSprite(
            ot, 0x4C, 0x140 - slide, 0x18, 0x10,
            0xB4, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);
        DrawSprite(
            ot, 0x4C, 0x150 - slide, 0x18, 0x10,
            0xCC, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);
        DrawSprite(
            ot, 0x4C, 0x160 - slide, 0x18, 0x10,
            0xE4, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);

        if (CourseSelectClassIndexValid(g_GrandPrixClass)) {
            prizeOffset = slide - 0x140;
            for (row = 0; row < PRIZE_PLACE_COUNT; row++) {
                coordinateY = row * 0x10 - prizeOffset;
                digitCount = GameDrawNumber(
                    0x65, coordinateY,
                    DRAW_NUMBER_LARGE_DIGITS | DRAW_NUMBER_OVERLAY_LAYER,
                    g_PrizeMoney.values[course][g_GrandPrixClass][row],
                    fade, fade, fade, 0x244, 0x20);
                DrawSprite(
                    ot, digitCount * 8 + 0x65, coordinateY, 0xC, 0x10,
                    0xF4, 0x28, fade, fade, fade,
                    0x244, 0, 1, 0x3B);
            }
        }
    }

    return g_CourseSelectScrollProgress;
}

/* Screen-fade callback used by the host menu renderer. */
s32 DrawRankingScreen(s32 step) {
    return AdvanceMenuFade(&g_RankingScrollState, step);
}
