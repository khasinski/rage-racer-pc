#include <stdio.h>
#include "game/prim.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/player_car_internal.h"

void DrawResultScreen(void) {
    u8 *base;
    u8 **cursorSlot;
    s32 width;
    s32 y;
    u8 *next;

    DrawProportionalText(0xDC, 0x1C, g_TextResult, 0x7812);

    if (g_GrandPrixMode != 0) {
        y = 0x3C;
    } else {
        y = 0x39;
    }
    DrawText8x8Trans(0x60, y, g_CourseNames[g_CourseIndex], 0x78CC);

    width = 0x140;
    base = (u8 *)GamePrimaryOrderingTable(0);
    cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);

    next = *cursorSlot;
    next = AddTilePrim(base, next, 0, 0, width, 0x30, 0x85, 0x15, 0xE);
    *cursorSlot = AddTilePrim(base, next, 0, 0x30, width, 0x18, 0xF0, 0xF0, 0xF0);
}

void DrawGrandPrixIntro(void) {
    u8 *base;
    char text[0x30];
    if ((g_ClassResultPlace != 0) &&
        (g_PrizeScreenState >= PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM)) {
        u8 **cursorSlot;
        u8 *next;
        s32 height;
        s32 color;

        cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);
        base = (u8 *)GamePrimaryOrderingTable(0);
        height = 8;
        color = 0x78CB;
        next = GameQueueSprite(
            base, *cursorSlot, 0x14, 0x1C, 0x38, height, 0, 0xE8, color);
        next = GameQueueSprite(
            base,
            next,
            0x4C,
            0x1C,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right,
            height,
            0x84,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].left,
            color);
        next = GameQueueSprite(
            base,
            next,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right + 0x4E,
            0x1C,
            0x30,
            height,
            0,
            0xF0,
            color);
        next = GameQueueSprite(
            base,
            next,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right + 0x7C,
            0x1C,
            0x20,
            height,
            0,
            0xF8,
            color);
        *cursorSlot = next;
    }

    {
        char *name;
        s32 classNumber;
        s32 current;

        current = g_GrandPrixClass;
        classNumber = current + 1;
        name = g_GrandPrixNames[g_GrandPrixSeries ? current + 6 : current];
        sprintf(text, g_FmtClassGrandPrix, classNumber, name);
    }
    DrawText8x8Trans(0x10, 0x34, text, 0x78CC);

    sprintf(text, g_FmtRoundIn, g_GrandPrixRound);
    DrawText8x8Trans(0x10, 0x3C, text, 0x78CC);

    {
        u8 **cursorSlot;
        u8 *next;
        s32 place;

        cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);
        DrawResultScreen();

        base = (u8 *)GamePrimaryOrderingTable(0);
        place = g_RacePosition;
        next = GameQueueSprite(
            base,
            *cursorSlot,
            0xB4,
            0x60,
            0x58,
            0x38,
            0xA8,
            0xA8,
            g_ResultPanelCluts[place]);

        next = GameQueueSprite(
            base,
            next,
            g_ResultPlaceSprites[place - 1].x,
            0x5C,
            g_ResultPlaceSprites[place - 1].y,
            0x1C,
            g_ResultPlaceSprites[place - 1].width,
            0xCC,
            g_ResultPlaceCluts[place]);
        *cursorSlot = next;
    }

    DrawProportionalText(0x10, 0x50, g_CaptionRanking, 0x7812);
}

void DrawRaceTimePanel(s32 slideY) {
    s32 i;
    s32 count;
    char text[24];
    s32 color;

    DrawProportionalText(0x10, slideY + 0x80, g_CaptionTotalTime, 0x7812);

    text[0] = 0x54;
    text[1] = 0x2F;
    FormatLapTime(&text[2], g_RaceTotalTime);

    color = 0x7812;
    if (g_BestTotalTimes[g_GrandPrixSeries][SeriesCourseIndex()][g_GrandPrixMode] == g_RaceTotalTime) {
        color = 0x784C;
    }
    DrawProportionalText(0x14, slideY + 0x90, text, color);

    DrawProportionalText(0x10, slideY + 0xA4, g_CaptionLapTime, 0x7812);

    count = g_CourseIndex == 3 ? 6 : 3;
    for (i = 0; i < count; i++) {
        s32 x = i < 3 ? 0x14 : 0xB0;
        s32 y = slideY + 0xB0 + (i % 3) * 0xC;

        text[0] = i + 0x31;
        FormatLapTime(&text[2],
                      g_PlayerCar.lapTimes.table.milliseconds[i]);
        color = g_PlayerCar.drive.hudLapHighlightRow == i ? 0x784C : 0x7812;
        DrawProportionalText(x, y, text, color);
    }
}
