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
ResultPlaceBarPosition g_ClassPlaceBarSizes[3];
ResultPlaceSpriteLayout g_ResultPlaceSprites[3];
u16 g_ResultPanelCluts[4];
u16 g_ResultPlaceCluts[4];

static const char *s_courseNameDrawn;

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
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    return packet + sizeof(SPRT);
}

int main(void) {
    u8 packets[sizeof(TILE) * 2];

    g_RenderState.packetCursor = packets;
    g_CourseIndex = 5;
    DrawCourseIntro();
    if (s_courseNameDrawn != g_CourseNames[1]) {
        fprintf(stderr, "Extra GP result used the wrong course name\n");
        return 1;
    }
    puts("result screen maps physical courses to name slots");
    return 0;
}
