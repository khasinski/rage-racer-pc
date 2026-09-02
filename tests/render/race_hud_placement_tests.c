/*
 * Where every part of the race HUD ends up, in both pictures.
 *
 * The widescreen layout pushes the HUD out to the edges, and it does that in
 * several places: the builder positions the sprites, DrawRaceHudLabels
 * repositions the ones it draws itself, and the times are drawn straight from
 * coordinates by the split and lap passes. A sprite that misses one of those
 * sits where a 4:3 screen put it while everything around it has moved, which
 * is what kept being reported and what kept being hard to see.
 *
 * So this asks the whole HUD, not one builder: run every pass in both
 * aspects, collect where each piece landed, and require that widening the
 * picture moved each piece by the margin its edge calls for and by nothing
 * else. That holds whatever the authored coordinates are, so it keeps holding
 * when they change.
 *
 * It needs no GPU and no disc: the drawing code builds primitives in memory,
 * and the positions are the whole question.
 */

#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "port_config.h"
#include "rage/hud_config.h"

#include <stdio.h>
#include <string.h>

void BuildRaceHudPrims(s32 mode);

enum {
    HUD_MARGIN = 53,
    HUD_CANVAS = 320,
    HUD_EDGE_BAND = 80,
    MAX_PLACEMENTS = 64
};

static RagePortConfig s_config;
static const char *s_anchor = "edges";
static int s_failures;

const RagePortConfig *PortActiveConfig(void) { return &s_config; }
int ModernIsEnabled(void) { return 1; }
const char *RuntimeConfigGet(const char *key) {
    if (strcmp(key, "hud.anchor") == 0) return s_anchor;
    return NULL;
}
int RuntimeConfigEnabled(const char *key) {
    const char *value = RuntimeConfigGet(key);
    return value != NULL && strcmp(value, "false") != 0;
}

/* The game's state the HUD reads. */
GameFrameContext g_FrameContexts[2];
GameFrameContext *g_DrawBuffer;
s32 g_FrameParity;
GameRenderState g_RenderState;
u8 g_DrawModeEnv[8];
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s32 g_LapCount;
s32 g_BestLapThisRace;
s32 g_LapTimeMs;
s32 g_SectorIndex;
s32 g_LastSectorTime;
s32 g_SplitDelta;
s16 g_SplitSign;
s16 g_SplitSector;
s32 g_SplitTargetTime;
s16 g_SplitTimer;
s32 g_RaceSeries;
s32 g_BestTotalTimes[2][4][2];
PlayerCarRuntime g_PlayerCar;
GameSpriteDesc g_TachoNeedleSprite;
s32 g_CourseIndex;

/*
 * The authored HUD, copied from the retail tables. Only the x matters here,
 * but the whole row is kept so the rows read as what they are.
 */
GameSpriteDesc g_RaceHudSpriteDescsTimeTrial[11] = {
    {240, 46, 8, 8, 0x50, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 56, 8, 8, 0x58, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 66, 8, 8, 0x60, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 76, 8, 8, 0x68, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 86, 8, 8, 0x70, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 96, 8, 8, 0x78, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {244, 12, 72, 16, 0x00, 0, 0x18, 0, 0x7809, {0, 0}, 0},
    {8, 12, 80, 16, 0x48, 0, 0x18, 0, 0x7803, {0, 0}, 0},
    {244, 112, 72, 8, 0xA0, 0, 0x30, 0, 0x7893, {0, 0}, 0},
    {8, 32, 8, 8, 0x50, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    /* The split time's sign, the one HUD sprite in the middle of the
     * picture rather than along an edge. */
    {120, 80, 8, 8, 0x78, 0, 0x08, 0, 0x78CC, {0, 0}, 0},
};

GameSpriteDesc g_RaceHudSpriteDescsGp[12] = {
    {240, 46, 8, 8, 0x50, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 56, 8, 8, 0x58, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 66, 8, 8, 0x60, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 76, 8, 8, 0x68, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 86, 8, 8, 0x70, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {240, 96, 8, 8, 0x78, 0, 0x40, 0, 0x78CC, {0, 0}, 0},
    {8, 12, 56, 16, 0x98, 0, 0x18, 0, 0x7809, {0, 0}, 0},
    {244, 12, 72, 16, 0x00, 0, 0x18, 0, 0x784D, {0, 0}, 0},
    {8, 198, 56, 8, 0xA0, 0, 0x38, 0, 0x7893, {0, 0}, 0},
    {6, 30, 24, 32, 0x18, 0, 0x48, 0, 0x780E, {0, 0}, 0},
    {30, 30, 24, 32, 0x18, 0, 0x48, 0, 0x780E, {0, 0}, 0},
    {54, 46, 24, 16, 0xE8, 0, 0x18, 0, 0x780A, {0, 0}, 0},
};

/*
 * The times are drawn from coordinates rather than from a sprite, so the only
 * way to see where they went is to be the drawing. Recording them here also
 * puts the value on record, which is how the magnitude check below works.
 */
typedef struct Placement {
    const char *what;
    int x;
    int y;
    int value;
} Placement;

static Placement s_placements[MAX_PLACEMENTS];
static int s_placementCount;

static void Record(const char *what, int x, int y, int value) {
    if (s_placementCount >= MAX_PLACEMENTS) return;
    s_placements[s_placementCount].what = what;
    s_placements[s_placementCount].x = x;
    s_placements[s_placementCount].y = y;
    s_placements[s_placementCount].value = value;
    s_placementCount++;
}

void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor) {
    (void)color;
    (void)divisor;
    Record("time", (int)x, (int)y, (int)value);
}

void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color) {
    (void)color;
    Record("clock", (int)x, (int)y, (int)ticks);
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim;
}

static void ResetHud(void) {
    memset(g_FrameContexts, 0, sizeof(g_FrameContexts));
    s_placementCount = 0;
    g_DrawBuffer = &g_FrameContexts[0];
    g_FrameParity = 0;
    /* No ordering table is wound up here. AddPrim only writes links into
     * memory, and nothing walks them: the positions live in the frame
     * context and in what the drawing passes were asked to draw. Clearing a
     * table for real needs the GPU brought up, which would cost this test
     * the machines it can run on. */
    g_RenderState.packetCursor = g_FrameContexts[0].layout.primitiveBuffer;
}

/* One full HUD, every pass a race runs, at values that put something in each
 * of them. */
static void DrawWholeHud(s32 mode) {
    int lap;

    ResetHud();
    g_GrandPrixMode = (s16)mode;
    g_LapCount = 3;
    g_PlayerCar.lap = 3;
    g_PlayerCar.drive.hudLapHighlightRow = 2;
    g_BestLapThisRace = 91875;
    g_PlayerCar.drive.racePosition = 12;  /* a macro onto the player car, not a global */
    for (lap = 0; lap < 6; lap++)
        g_PlayerCar.lapTimes.table.milliseconds[lap] = 95000 + lap * 1234;
    g_SplitTimer = 0;
    g_SectorIndex = 1;
    g_SplitSign = -1;
    g_SplitDelta = 1200;
    g_SplitSector = 1;
    g_LapTimeMs = 92345;
    g_LastSectorTime = 31450;
    g_SplitTargetTime = 91000;
    g_RaceSeries = 0;
    g_BestTotalTimes[0][0][0] = 278900;

    BuildRaceHudPrims(mode);
    DrawLapTimes();
    DrawRaceHudLabels(mode);
    if (mode != 0) DrawRacePosition();
    DrawSplitTimes();
    DrawTimeRemaining(4500);
}

/* Which edge an authored coordinate belongs to, and so how far widening the
 * picture is allowed to move it. */
static int ExpectedShift(int authoredX) {
    if (authoredX < HUD_EDGE_BAND) return -HUD_MARGIN;
    if (authoredX >= HUD_CANVAS - HUD_EDGE_BAND) return HUD_MARGIN;
    return 0;
}

static void Fail(const char *what, int index, int narrow, int wide,
                 int expected) {
    printf("FAIL %s %d: 4:3 x=%d, 16:9 x=%d, moved %d, expected %d\n", what,
           index, narrow, wide, wide - narrow, expected);
    s_failures++;
}

static void CheckSprites(s32 mode, const int *narrow, const int *wide) {
    s32 rowCount = mode != 0 ? 12 : 11;
    s32 row;
    for (row = 0; row < rowCount; row++) {
        int expected = ExpectedShift(narrow[row]);
        if (wide[row] - narrow[row] != expected)
            Fail(mode != 0 ? "gp sprite" : "time-trial sprite", (int)row,
                 narrow[row], wide[row], expected);
    }
}

static void CollectSprites(s32 mode, int *out) {
    GameFrameContext *frame = g_DrawBuffer;
    s32 rowCount = mode != 0 ? 12 : 11;
    s32 row;
    for (row = 0; row < rowCount; row++)
        out[row] = row < 6 ? frame->layout.raceHud.lapTimes[row].x0
                           : frame->layout.raceHud.labels[row - 6].x0;
}

static void CheckMode(s32 mode) {
    const char *name = mode != 0 ? "grand prix" : "time trial";
    int narrowSprites[12], wideSprites[12];
    Placement narrowTimes[MAX_PLACEMENTS];
    int narrowCount;
    int index;

    s_config.modernAspect = RAGE_MODERN_ASPECT_4_3;
    DrawWholeHud(mode);
    CollectSprites(mode, narrowSprites);
    memcpy(narrowTimes, s_placements, sizeof(narrowTimes));
    narrowCount = s_placementCount;

    s_config.modernAspect = RAGE_MODERN_ASPECT_16_9;
    DrawWholeHud(mode);
    CollectSprites(mode, wideSprites);

    CheckSprites(mode, narrowSprites, wideSprites);

    if (s_placementCount != narrowCount) {
        printf("FAIL %s: %d times drawn at 4:3 but %d at 16:9\n", name,
               narrowCount, s_placementCount);
        s_failures++;
        return;
    }
    for (index = 0; index < narrowCount; index++) {
        int expected = ExpectedShift(narrowTimes[index].x);
        if (s_placements[index].x - narrowTimes[index].x != expected)
            Fail(narrowTimes[index].what, index, narrowTimes[index].x,
                 s_placements[index].x, expected);
        if (s_placements[index].y != narrowTimes[index].y) {
            printf("FAIL %s %d: widening moved it down from y=%d to y=%d\n",
                   narrowTimes[index].what, index, narrowTimes[index].y,
                   s_placements[index].y);
            s_failures++;
        }
    }
}

/*
 * The split delta is stored as a magnitude with its direction in the sign,
 * because DrawTimeValue reads a negative value as "no time yet" and draws
 * dashes. Handing it a signed delta therefore replaces the number with
 * -'--"--- rather than showing a gap the wrong way round, which is the kind
 * of thing that looks like a missing feature instead of a bug.
 */
static void CheckSplitDeltaIsAMagnitude(void) {
    int index;
    s_config.modernAspect = RAGE_MODERN_ASPECT_4_3;
    DrawWholeHud(0);
    for (index = 0; index < s_placementCount; index++) {
        if (s_placements[index].value >= 0) continue;
        printf("FAIL time %d at x=%d drawn with a negative value %d, which "
               "draws as dashes\n",
               index, s_placements[index].x, s_placements[index].value);
        s_failures++;
    }
}

/* The sign sits beside the number it belongs to, and neither is at an edge:
 * anchoring must leave both of them where they are. */
static void CheckSplitSignStaysWithItsTime(void) {
    GameFrameContext *frame;
    int signX;
    int timeX = -1;
    int index;

    s_config.modernAspect = RAGE_MODERN_ASPECT_16_9;
    DrawWholeHud(0);
    frame = g_DrawBuffer;
    signX = frame->layout.raceHud.labels[4].x0;
    for (index = 0; index < s_placementCount; index++)
        if (s_placements[index].y == 0x50) timeX = s_placements[index].x;

    if (timeX < 0) {
        printf("FAIL the split delta was not drawn at all\n");
        s_failures++;
        return;
    }
    if (signX != g_RaceHudSpriteDescsTimeTrial[10].x) {
        printf("FAIL split sign moved to %d; it is mid-picture and should "
               "stay at %d\n",
               signX, (int)g_RaceHudSpriteDescsTimeTrial[10].x);
        s_failures++;
    }
    if (timeX - signX != 8) {
        printf("FAIL split sign at %d is %d from its time at %d, expected 8\n",
               signX, timeX - signX, timeX);
        s_failures++;
    }
}

static void CheckRacePositionDigits(void) {
    GameFrameContext *frame;

    s_config.modernAspect = RAGE_MODERN_ASPECT_4_3;
    ResetHud();
    BuildRaceHudPrims(1);
    frame = g_DrawBuffer;

    g_PlayerCar.drive.racePosition = 3;
    DrawRacePosition();
    if (frame->layout.raceHud.labels[3].u0 != 0 ||
        frame->layout.raceHud.labels[4].u0 != 3 * 24 ||
        frame->layout.raceHud.labels[3].clut != 0x780B ||
        frame->layout.raceHud.labels[4].clut != 0x780B) {
        printf("FAIL podium race-position digits or colors\n");
        s_failures++;
    }

    g_PlayerCar.drive.racePosition = 12;
    DrawRacePosition();
    if (frame->layout.raceHud.labels[3].u0 != 0x18 ||
        frame->layout.raceHud.labels[4].u0 != 2 * 24 ||
        frame->layout.raceHud.labels[3].clut != 0x780E ||
        frame->layout.raceHud.labels[4].clut != 0x780E) {
        printf("FAIL two-digit race-position digits or colors\n");
        s_failures++;
    }
}

static void CheckSplitDeltaSprites(void) {
    GameFrameContext *frame;

    ResetHud();
    BuildRaceHudPrims(0);
    frame = g_DrawBuffer;

    DrawSplitDelta(7, 1);
    if (frame->layout.raceHud.labels[3].u0 != 7 * 8 + 0x50 ||
        frame->layout.raceHud.labels[4].u0 != 0x88 ||
        frame->layout.raceHud.labels[4].clut != 0x7810) {
        printf("FAIL positive split-delta sprites\n");
        s_failures++;
    }

    DrawSplitDelta(2, -1);
    if (frame->layout.raceHud.labels[3].u0 != 2 * 8 + 0x50 ||
        frame->layout.raceHud.labels[4].u0 != 0x78 ||
        frame->layout.raceHud.labels[4].clut != 0x780F) {
        printf("FAIL negative split-delta sprites\n");
        s_failures++;
    }
}

int main(void) {
    CheckMode(0);
    CheckMode(1);
    CheckSplitDeltaIsAMagnitude();
    CheckSplitSignStaysWithItsTime();
    CheckRacePositionDigits();
    CheckSplitDeltaSprites();
    if (s_failures != 0) {
        printf("race_hud_placement: %d failures\n", s_failures);
        return 1;
    }
    printf("race_hud_placement: ok\n");
    return 0;
}
