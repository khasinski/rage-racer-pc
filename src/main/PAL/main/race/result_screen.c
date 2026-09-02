#include <stdio.h>
#include "game/prim.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/race_internal.h"

void DrawResultScreen(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 courseNameY = g_GrandPrixMode != 0 ? 0x3C : 0x39;
    u8 *next;

    DrawProportionalText(0xDC, 0x1C, g_TextResult, 0x7812);
    DrawText8x8Trans(0x60, courseNameY, g_CourseNames[g_CourseIndex],
                     0x78CC);

    next = AddTilePrim(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0,
                       0x140, 0x30, 0x85, 0x15, 0xE);
    RENDER_PRIM_CURSOR_AS(u8) = AddTilePrim(
        ot, next, 0, 0x30, 0x140, 0x18, 0xF0, 0xF0, 0xF0);
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
    RENDER_PRIM_CURSOR_AS(u8) = GameQueueSprite(
        ot, next, bar->right + 0x7C, 0x1C, 0x20, 8,
        0, 0xF8, 0x78CB);
}

static void DrawResultPlace(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    const ResultPlaceSpriteLayout *placeSprite =
        &g_ResultPlaceSprites[g_RacePosition - 1];
    u8 *next;

    DrawResultScreen();
    next = GameQueueSprite(ot, RENDER_PRIM_CURSOR_AS(u8),
                           0xB4, 0x60, 0x58, 0x38, 0xA8, 0xA8,
                           g_ResultPanelCluts[g_RacePosition]);
    RENDER_PRIM_CURSOR_AS(u8) = GameQueueSprite(
        ot, next, placeSprite->x, 0x5C, placeSprite->y, 0x1C,
        placeSprite->width, 0xCC, g_ResultPlaceCluts[g_RacePosition]);
}

void DrawGrandPrixIntro(void) {
    char text[0x30];
    s32 classIndex = g_GrandPrixClass;
    char *grandPrixName =
        g_GrandPrixNames[g_GrandPrixSeries ? classIndex + 6 : classIndex];

    if ((g_ClassResultPlace != 0) &&
        (g_PrizeScreenState >= PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM)) {
        DrawClassPlaceBanner();
    }

    sprintf(text, g_FmtClassGrandPrix, classIndex + 1, grandPrixName);
    DrawText8x8Trans(0x10, 0x34, text, 0x78CC);

    sprintf(text, g_FmtRoundIn, g_GrandPrixRound);
    DrawText8x8Trans(0x10, 0x3C, text, 0x78CC);

    DrawResultPlace();
    DrawProportionalText(0x10, 0x50, g_CaptionRanking, 0x7812);
}
