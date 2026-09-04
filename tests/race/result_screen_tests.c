#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/screens.h"

#include <stdio.h>

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
PlayerCarRuntime g_PlayerCar;
s32 g_CourseIndex;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_GrandPrixClass;
s32 g_GrandPrixRound;
s32 g_ClassResultPlace;
PrizeScreenState g_PrizeScreenState;
char *g_CourseNames[COURSE_SLOT_COUNT] = {
    "COURSE 0", "COURSE 1", "COURSE 2", "COURSE 3",
};
char *g_GrandPrixNames[11];
char g_TextResult[] = "RESULT";
char g_TextTimeAttack[] = "TIME ATTACK";
char g_TextCourseIn[] = "COURSE IN";
char g_CaptionRanking[] = "RANKING";
char g_FmtClassGrandPrix[] = "CLASS%d %s GRANDPRIX";
char g_FmtRoundIn[] = "ROUND%d IN";
ResultPlaceBarTable g_ClassPlaceBarSizes;
ResultPlaceSpriteTable g_ResultPlaceSprites;
ResultPanelClutTable g_ResultPanelCluts;
u16 g_ResultPlaceCluts[4];

static const char *s_courseNameDrawn;
static s32 s_spriteCount;
static s32 s_spriteX[4];
static s32 s_spriteU[4];
static s32 s_spriteWidth[4];
static s32 s_spriteClut[4];

void DrawProportionalText(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
}

void DrawText8x8Trans(s32 x, s32 y, const char *text, s32 clut) {
    (void)y;
    (void)clut;
    if (x == 0x60) {
        s_courseNameDrawn = text;
    }
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                s32 width, s32 height, s32 red, s32 green, s32 blue) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    return packet + sizeof(TILE);
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                    s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)y;
    (void)height;
    (void)v;
    if (s_spriteCount < 4) {
        s_spriteX[s_spriteCount] = x;
        s_spriteU[s_spriteCount] = u;
        s_spriteWidth[s_spriteCount] = width;
        s_spriteClut[s_spriteCount] = clut;
    }
    s_spriteCount++;
    return packet + sizeof(SPRT);
}

int main(void) {
    u8 packets[sizeof(TILE) * 2 + sizeof(SPRT) * 2];

    g_RenderState.packetCursor = packets;
    g_CourseIndex = 5;
    DrawCourseIntro();
    if (s_courseNameDrawn != g_CourseNames[1]) {
        fprintf(stderr, "Extra GP result used the wrong course name\n");
        return 1;
    }

    g_RenderState.packetCursor = packets;
    g_GrandPrixMode = 1;
    g_GrandPrixSeries = 0;
    g_GrandPrixClass = 0;
    g_GrandPrixRound = 1;
    g_ClassResultPlace = 0;
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INVALID;
    g_GrandPrixNames[0] = "GP";
    g_PlayerCar.drive.racePosition = 2;
    g_ResultPlaceSprites.places[1] =
        (ResultPlaceSpriteLayout){.x = 26, .width = 64, .u = 48};
    g_ResultPanelCluts.byPlace[2] = 0x1234;
    g_ResultPlaceCluts[2] = 0x5678;
    s_spriteCount = 0;
    DrawGrandPrixIntro();
    if (s_spriteCount != 2 || s_spriteClut[0] != 0x1234 ||
        s_spriteX[1] != 26 || s_spriteWidth[1] != 64 ||
        s_spriteU[1] != 48 || s_spriteClut[1] != 0x5678) {
        fprintf(stderr, "result place did not use its typed table row\n");
        return 1;
    }
    puts("result screen maps physical courses to name slots");
    return 0;
}
