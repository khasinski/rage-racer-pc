#include <stdio.h>
#include "game/prim.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/race_internal.h"

enum { RESULT_INTRO_TEXT_CAPACITY = 48 };

void DrawResultScreen(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 courseNameY = ResultCourseNameY(g_GrandPrixMode);
    u8 *next;

    DrawProportionalText(0xDC, 0x1C, g_TextResult, 0x7812);
    DrawText8x8Trans(0x60, courseNameY, g_CourseNames[g_CourseIndex],
                     0x78CC);

    next = AddTilePrim(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0,
                       0x140, 0x30, 0x85, 0x15, 0xE);
    g_RenderState.packetCursor = AddTilePrim(
        ot, next, 0, 0x30, 0x140, 0x18, 0xF0, 0xF0, 0xF0);
}

void DrawCourseIntro(void) {
    DrawProportionalText(0x10, 0x1C, g_TextTimeAttack, 0x7812);
    DrawText8x8Trans(0x10, 0x39, g_TextCourseIn, 0x78CC);
    DrawResultScreen();
}

static void DrawClassPlaceBanner(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    const ResultPlaceBarPosition *bar =
        &g_ClassPlaceBarSizes[g_ClassResultPlace - 1];
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);

    next = GameQueueSprite(ot, next, 0x14, 0x1C, 0x38, 8,
                           0, 0xE8, 0x78CB);
    next = GameQueueSprite(ot, next, 0x4C, 0x1C, bar->right, 8,
                           0x84, bar->left, 0x78CB);
    next = GameQueueSprite(ot, next, bar->right + 0x4E, 0x1C,
                           0x30, 8, 0, 0xF0, 0x78CB);
    g_RenderState.packetCursor = GameQueueSprite(
        ot, next, bar->right + 0x7C, 0x1C, 0x20, 8,
        0, 0xF8, 0x78CB);
}

static void DrawResultPlace(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 racePosition = g_PlayerCar.drive.racePosition;
    const ResultPlaceSpriteLayout *placeSprite;
    u8 *next;

    DrawResultScreen();
    if (!IsValidRaceResultPlace(racePosition)) {
        return;
    }
    placeSprite = &g_ResultPlaceSprites[racePosition - 1];
    next = GameQueueSprite(ot, RENDER_PRIM_CURSOR_AS(u8),
                           0xB4, 0x60, 0x58, 0x38, 0xA8, 0xA8,
                           g_ResultPanelCluts[racePosition]);
    g_RenderState.packetCursor = GameQueueSprite(
        ot, next, placeSprite->x, 0x5C, placeSprite->y, 0x1C,
        placeSprite->width, 0xCC, g_ResultPlaceCluts[racePosition]);
}

void DrawGrandPrixIntro(void) {
    char text[RESULT_INTRO_TEXT_CAPACITY];
    s32 classIndex = g_GrandPrixClass;
    s32 nameIndex = GrandPrixNameIndex(g_GrandPrixSeries, classIndex);
    const char *grandPrixName =
        nameIndex >= 0 ? g_GrandPrixNames[nameIndex] : "";

    if (ShouldDrawClassPlaceBanner(g_ClassResultPlace, g_PrizeScreenState)) {
        DrawClassPlaceBanner();
    }

    snprintf(text, sizeof(text), g_FmtClassGrandPrix, classIndex + 1,
             grandPrixName);
    DrawText8x8Trans(0x10, 0x34, text, 0x78CC);

    snprintf(text, sizeof(text), g_FmtRoundIn, g_GrandPrixRound);
    DrawText8x8Trans(0x10, 0x3C, text, 0x78CC);

    DrawResultPlace();
    DrawProportionalText(0x10, 0x50, g_CaptionRanking, 0x7812);
}
