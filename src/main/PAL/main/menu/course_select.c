#include "game/asset.h"
#include "game/audio.h"
#include "game/course_select_internal.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/race.h"

const char g_NowLoadingText[] = "NOW LOADING";
/* Blinks the "NOW LOADING" string at g_NowLoadingText. */
void DrawNowLoadingText(void) {
    if (g_SceneTimer & 8) {
        DrawText8x8(0x74, 0xEC, g_NowLoadingText, 0x78CC);
    }
}


/* g_MenuScreenUpdate[0]: waits for the car-select assets, then opens screen 1. */
void EnterCourseSelectScreen(void) {
    s32 initValue;
    s32 mode;
    s32 largeValue;
    u8 *table;
    s32 eight;

    DrawNowLoadingText();
    if (RequestCarSelectAssets() != 0) {
        return;
    }

    PlaySequence();
    g_MenuHandlerIndex = 1;
    g_MenuScreen = 1;
    DrawBrowseArrows(0, 0, 0, 0);

    initValue = 0x7A120;
    mode = 0x3D090;
    largeValue = 0x1F0000;
    g_MenuViewOffset = mode;
    mode = g_CourseIndex;
    eight = 8;
    g_MenuViewSpin = eight;
    table = g_CourseProgress->bestPlace;
    largeValue |= 0x4000;
    g_UiScriptProgress = 0;
    g_PlayerCar.x = 0;
    g_PlayerCar.y = 0;
    g_PlayerCar.z = 0;
    g_PlayerCar.bodyPitch = 0;
    g_PlayerCar.bodyYaw = 0;
    g_PlayerCar.bodyRoll = 0;
    g_PlayerCar.trackProgress = 0;
    g_PlayerSteerAngle = 0;
    g_PlayerCarWheelAngle = 0;
    g_MenuViewAngleTarget = 0x7A120;
    g_MenuViewAngle = initValue;
    g_MenuViewOffsetTarget = 0;
    g_CourseCardSpin = largeValue;
    g_CourseCardSpinTarget = 0;
    g_CourseCardPendingGrade = table[mode & 3];

    if (mode >= 4) {
        g_TimeAttackPlateStep = 1;
    } else {
        g_TimeAttackPlateStep = -1;
    }

    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
}

typedef struct CourseSelectSpriteBounds {
    s32 x;
    s32 y;
    s32 w;
    s32 h;
} CourseSelectSpriteBounds;


/*
 * The menu lays out several two-sprite labels from the first sprite's sliding
 * Y. GCC 2.6.3 integrates this explicit inline at -O2; its optional bounds
 * result also preserves the retail caller frame before null outputs fold away.
 */
static s32 GameDrawSlidingSprite(
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
    s32 flags,
    CourseSelectSpriteBounds *bounds)
{
    CourseSelectSpriteBounds result;
    s32 y;

    y = baseY - slide;
    if (bounds != 0) {
        result.x = x;
        result.y = y;
        result.w = w;
        result.h = h;
        *bounds = result;
    }
    DrawSprite(
        ot, x, y, w, h, u, v, r, g, b, clut,
        shadeTex, semiTrans, flags);
    return y;
}

s32 DrawCourseSelectScreen(s32 step)
{
    OT_TYPE *otBase;
    OT_TYPE *ot;
    u8 fade;
    u16 slide;
    s16 headerWidth = 0;
    u32 deltaY;
    /* Load-bearing in the prize loop: without this pin the function is 848 words. */
    s32 coordinateY;
    s32 lineColor;
    s32 row;
    s32 digitCount;
    s32 prizeOffset;
    GrandPrixPrizeTable *prizeTable;
    s32 prizeFade;
    s32 prizeClut;
    s32 gpHeight;
    s32 gpClut;
    s32 gpSemiTrans;
    s32 gpFlags;
    s32 gpSlide;
    u32 gpFade;
    u32 fadeValue;
    OrderingTableAddress otAddress;

    otBase = SCRATCH_OT_BASE_AS(OT_TYPE);
    ot = otBase + 1;
    if (step == 0) {
        g_CourseSelectScrollValue = 0;
        otAddress.pointer = otBase;
        return otAddress.value;
    }

    if (step > 0) {
        g_CourseSelectScrollValue += step;
        if (g_CourseSelectScrollValue >= 0x1FD) {
            g_CourseSelectScrollValue = 0x1FC;
        }
        slide = 0;
    } else {
        g_CourseSelectScrollValue += step;
        if (g_CourseSelectScrollValue < 0) {
            g_CourseSelectScrollValue = 0;
        }
        deltaY = 0x1FC - g_CourseSelectScrollValue;
        slide = (u16)(deltaY * deltaY / 2048);
    }

    if (g_MenuAltLayout != 0) {
        return g_CourseSelectScrollState.value;
    }

    slide -= 0x28;
    fadeValue = g_CourseSelectScrollValue;
    fade = (u8)(fadeValue / 4);

    if (g_GrandPrixMode != 0) {
        if (g_SeriesSelection == 0) {
            switch (g_GrandPrixClass) {
            case 0:
                headerWidth = 0x24;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x24, 0x10,
                    0, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 1:
                headerWidth = 0x20;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x20, 0x10,
                    0x24, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 2:
                headerWidth = 0x28;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x28, 0x10,
                    0x44, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 3:
                headerWidth = 0x30;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x30, 0x10,
                    0x6C, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 4:
                headerWidth = 0x30;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x30, 0x10,
                    0x9C, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            }
        } else {
            switch (g_GrandPrixClass) {
            case 0:
                headerWidth = 0x30;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x30, 0x10,
                    0xCC, 0x38, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 1:
                headerWidth = 0x40;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x40, 0x10,
                    0, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 2:
                headerWidth = 0x3C;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x3C, 0x10,
                    0x40, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 3:
                headerWidth = 0x28;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x28, 0x10,
                    0x7C, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 4:
                headerWidth = 0x20;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x20, 0x10,
                    0xA4, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            case 5:
                headerWidth = 0x28;
                DrawSprite(
                    ot, 0x50, 0xB0 - (s16)slide, 0x28, 0x10,
                    0xC4, 0x48, fade, fade, fade, 0x244, 0, 1, 0x5B);
                break;
            }
        }

        gpHeight = 0x10;
        gpClut = 0x244;
        gpSemiTrans = 1;
        gpSlide = (s16)slide;
        gpFade = fade;
        gpFlags = 0x5B;
        DrawSprite(
            ot, headerWidth + 0x50, 0xB0 - gpSlide, 0x10,
            gpHeight, 0xEC, 0x48, gpFade, gpFade, gpFade,
            gpClut, 0, gpSemiTrans, gpFlags);

        coordinateY = GameDrawSlidingSprite(
            ot, 0x50, 0x97, gpSlide, 0x1A, gpHeight, 0x60, 0xCC,
            gpFade, gpFade, gpFade, gpClut, 0, gpSemiTrans, gpFlags, 0);
        DrawSprite(
            ot, 0x6C, coordinateY, 8, 0x10,
            g_GrandPrixClass * 8 + 8, 0x18,
            gpFade, gpFade, gpFade, gpClut, 0, gpSemiTrans, gpFlags);

        lineColor = gpFade * 2;
        DrawLine(
            ot, 0x48, 0xAA - gpSlide, 0xAF, 0xAA - gpSlide,
            lineColor, lineColor, lineColor, 0x40);
        DrawLine(
            ot, 0x48, 0xAB - gpSlide, 0xAF, 0xAB - gpSlide,
            lineColor, lineColor, lineColor, 0x40);
        DrawSolidRect(
            ot, 0x48, 0x94 - gpSlide, 0x68, 0x30,
            lineColor, lineColor, lineColor,
            gpFade < 0x7F ? 0x20 : 0xFF);
        DrawSprite(
            ot, 0xB0, 0x94 - (s16)slide, 0x20, 0x30,
            0x60, 0x88, fade, fade, fade, 0x25B, 0, 1, 0x39);
    }

    coordinateY = GameDrawSlidingSprite(
        ot, 0x4C, 0xD0, (s16)slide, 0x18, 0xC, 0x18, 0xDC,
        fade, fade, fade, 0x244, 0, 1, 0x3A, 0);
    DrawSprite(
        ot, 0x68, coordinateY, 0x12, 0xC,
        0x32, 0xDC, fade, fade, fade, 0x244, 0, 1, 0x3A);

    coordinateY = GameDrawSlidingSprite(
        ot, 0x4C, 0xF8, (s16)slide, 0x18, 0xC, 0x18, 0xDC,
        fade, fade, fade, 0x244, 0, 1, 0x3A, 0);
    DrawSprite(
        ot, 0x68, coordinateY, 0x1A, 0xC,
        0x46, 0xDC, fade, fade, fade, 0x244, 0, 1, 0x3A);

    switch (SeriesCourseIndex()) {
    case 0:
        coordinateY = GameDrawSlidingSprite(
            ot, 0x4C, 0xE0, (s16)slide, 8, 0x10, 8, 0x18,
            fade, fade, fade, 0x244, 0, 1, 0x3B, 0);
        DrawSprite(
            ot, 0x54, coordinateY, 0x54, 0x10,
            0, 0x9C, fade, fade, fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x4C, 0x108 - (s16)slide, 0x20, 0x10,
            0x44, 0xB4, fade, fade, fade, 0x244, 0, 1, 0x3A);
        break;
    case 1:
        coordinateY = GameDrawSlidingSprite(
            ot, 0x4C, 0xE0, (s16)slide, 8, 0x10, 0x10, 0x18,
            fade, fade, fade, 0x244, 0, 1, 0x3B, 0);
        DrawSprite(
            ot, 0x54, coordinateY, 0x4C, 0x10,
            0x54, 0x9C, fade, fade, fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x4C, 0x108 - (s16)slide, 0x20, 0x10,
            0x64, 0xB4, fade, fade, fade, 0x244, 0, 1, 0x3A);
        break;
    case 2:
        coordinateY = GameDrawSlidingSprite(
            ot, 0x4C, 0xE0, (s16)slide, 8, 0x10, 0x18, 0x18,
            fade, fade, fade, 0x244, 0, 1, 0x3B, 0);
        DrawSprite(
            ot, 0x54, coordinateY, 0x48, 0x10,
            0, 0xAC, fade, fade, fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x4C, 0x108 - (s16)slide, 0x20, 0x10,
            0x84, 0xB4, fade, fade, fade, 0x244, 0, 1, 0x3A);
        break;
    case 3:
        coordinateY = GameDrawSlidingSprite(
            ot, 0x4C, 0xE0, (s16)slide, 8, 0x10, 0x20, 0x18,
            fade, fade, fade, 0x244, 0, 1, 0x3B, 0);
        DrawSprite(
            ot, 0x54, coordinateY, 0x5C, 0x10,
            0xA4, 0x9C, fade, fade, fade, 0x244, 0, 1, 0x3B);
        DrawSprite(
            ot, 0x4C, 0x108 - (s16)slide, 0x1E, 0x10,
            0xA4, 0xB4, fade, fade, fade, 0x244, 0, 1, 0x3A);
        break;
    }

    if (g_GrandPrixMode != 0) {
        DrawSprite(
            ot, 0x4C, 0x140 - (s16)slide, 0x18, 0x10,
            0xB4, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);
        DrawSprite(
            ot, 0x4C, 0x150 - (s16)slide, 0x18, 0x10,
            0xCC, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);
        DrawSprite(
            ot, 0x4C, 0x160 - (s16)slide, 0x18, 0x10,
            0xE4, 0xCC, fade, fade, fade, 0x244, 0, 1, 0x3A);

        row = 0;
        prizeOffset = (s16)slide - 0x140;
        prizeTable = &g_PrizeMoney;
        prizeFade = fade;
        prizeClut = 0x244;
        do {
            coordinateY = row * 0x10 - prizeOffset;
            digitCount = GameDrawNumber(
                0x65, coordinateY, 9,
                prizeTable->values[SeriesCourseIndex()][g_GrandPrixClass][row],
                prizeFade, prizeFade, prizeFade, prizeClut, 0x20);
            row++;
            DrawSprite(
                ot, digitCount * 8 + 0x65, coordinateY, 0xC, 0x10,
                0xF4, 0x28, prizeFade, prizeFade, prizeFade,
                prizeClut, 0, 1, 0x3B);
        } while (row < 3);
    }

    return g_CourseSelectScrollValue;
}

/* The mirror of CanSelectNextCourse. */
s32 CanSelectPrevCourse(void) {
    s32 v1 = 0;
    if (g_GrandPrixMode != 0) {
        v1 = (g_SeriesSelection != 0) << 2;
    }
    return v1 < g_CourseIndex;
}

s32 CanSelectNextCourse(void) {
    s32 limit;

    if (g_GrandPrixMode != 0) {
        if (g_SeriesSelection != 0) {
            limit = (g_GrandPrixClass < 2) ? 6 : 7;
        } else {
            limit = (g_GrandPrixClass < 2) ? 2 : 3;
        }
    } else if (g_ExtraGrandPrixUnlocked != 0) {
        limit = (g_MaxClassReached[1] < 2) ? 6 : 7;
    } else {
        limit = (g_MaxClassReached[0] < 2) ? 2 : 3;
    }

    return g_CourseIndex < limit;
}

s32 DrawRankingScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_RankingScrollState = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_RankingScrollState;
        g_RankingScrollState = value;
        if (value >= 0x1FD) {
            g_RankingScrollState = 0x1FC;
        }
    } else {
        value = step + g_RankingScrollState;
        g_RankingScrollState = value;
        if (value < 0) {
            g_RankingScrollState = 0;
        }
    }

    return g_RankingScrollState;
}
