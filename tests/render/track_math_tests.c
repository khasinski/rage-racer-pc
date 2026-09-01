/*
 * The small pure pieces the refactor pulled out of the track code.
 *
 * Each of these was checked once, by hand, against the version it replaced,
 * and then the check was thrown away. That is the wrong way round: the
 * properties are cheap to state and the functions are cheap to call, so the
 * checks belong here where they run every build.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/render.h"
#include "game/track.h"
#include "game/vector.h"

/* What the functions under test read and write. */
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_TrackTextureSectionLo;
s32 g_TrackTextureSectionHi;
s32 g_TrackTexturePageWanted;

/*
 * SelectTrackTexturePage is four lines and touches three of the words above.
 * Its file also holds the page uploader and two camera cyclers, so linking
 * it drags these in as well. They are named here and never used; the day
 * that file is split by subject, this block goes away.
 */
GameCarRuntime g_Cars[11];
s32 g_SceneTimer;
s32 g_TrackTextureCursorRow;
Rect g_TrackTextureRowRect;
u8 g_TrackTextureShadowPage[256];
TrackTextureShadowRow *g_TrackTextureShadow;
s32 g_TrackTextureTargetRow;
s32 Random15(void) { return 0; }

s32 SelectTrackTexturePage(s32 section);

static int s_failures;

static void Check(int ok, const char *what) {
    if (ok) return;
    s_failures++;
    printf("FAIL %s\n", what);
}

/*
 * The track is a ring. Every index into it wraps, including negative ones,
 * which is what the callers that wrote the modulo by hand kept getting wrong.
 */
static void TrackPointWraps(void) {
    GameTrackPoint points[4];
    s32 i;

    for (i = 0; i < 4; i++) points[i].angle = (s16)(100 + i);
    g_TrackPoints = points;
    g_TrackPointCount = 4;

    Check(TrackPoint(0) == &points[0], "index 0");
    Check(TrackPoint(3) == &points[3], "last index");
    Check(TrackPoint(4) == &points[0], "one past the end wraps");
    Check(TrackPoint(9) == &points[1], "several laps around wrap");
    Check(TrackPoint(-1) == &points[3], "negative index wraps backwards");
    Check(TrackPoint(-4) == &points[0], "a whole lap backwards wraps");
    Check(TrackPoint(-5) == &points[3], "more than a lap backwards wraps");

    /* A course whose points have not loaded must not divide by zero. */
    g_TrackPointCount = 0;
    Check(TrackPoint(7) == &points[0], "no points yet is not a crash");
    g_TrackPointCount = 4;
}

/*
 * DistanceXZ exists because squaring a coordinate difference overflows a
 * signed 32-bit int at a separation of about 46000, which put a negative
 * number into SquareRoot12 and, eventually, took the finish camera down.
 */
static void DistanceSurvivesLargeSeparations(void) {
    s32 near = DistanceXZ(3000, 4000);
    s32 far = DistanceXZ(300000, 400000);
    s32 justOver = DistanceXZ(46341, 0);

    Check(near > 0, "a short distance is positive");
    Check(far > 0, "a distance that would overflow s32 is still positive");
    Check(justOver > 0, "the old cliff at ~46341 is gone");
    Check(far > near, "the farther pair measures farther");
    Check(DistanceXZ(-3000, -4000) == near, "sign of the difference does not matter");
    Check(DistanceXZ(0, 0) == 0, "no distance is zero");
}

/*
 * One stretch of each course draws from a second page of track textures.
 * The answer is one comparison, and both of its outputs matter: the flag the
 * uploader watches and the page bit the caller ORs into its primitives.
 */
static void SecondTexturePageIsOneRange(void) {
    g_TrackTextureSectionLo = 10;
    g_TrackTextureSectionHi = 20;

    Check(SelectTrackTexturePage(9) == 0 && g_TrackTexturePageWanted == 0,
          "before the stretch");
    Check(SelectTrackTexturePage(10) == 0x100 && g_TrackTexturePageWanted == 1,
          "first section of the stretch");
    Check(SelectTrackTexturePage(19) == 0x100 && g_TrackTexturePageWanted == 1,
          "last section of the stretch");
    Check(SelectTrackTexturePage(20) == 0 && g_TrackTexturePageWanted == 0,
          "the upper bound is outside");
    Check(SelectTrackTexturePage(99) == 0 && g_TrackTexturePageWanted == 0,
          "past the stretch");

    /* A course with no second page at all: the bounds meet. */
    g_TrackTextureSectionHi = g_TrackTextureSectionLo;
    Check(SelectTrackTexturePage(10) == 0 && g_TrackTexturePageWanted == 0,
          "an empty stretch selects nothing");
}

static void TextureSwapStateResetsAndSkipsMatchingRows(void) {
    s32 row;

    memset(g_TrackTextureShadowPage, 0, sizeof(g_TrackTextureShadowPage));
    g_TrackTexturePageWanted = 1;
    g_TrackTextureTargetRow = 12;
    g_TrackTextureCursorRow = 34;
    ResetTrackTextureSwap();
    for (row = 0; row < 256; row++) {
        Check(g_TrackTextureShadowPage[row] == 1,
              "reset marks every shadow row as page one");
    }
    Check(g_TrackTexturePageWanted == 0, "reset wants the first page");
    Check(g_TrackTextureTargetRow == 0, "reset target row");
    Check(g_TrackTextureCursorRow == 0, "reset cursor row");

    /* A mismatched state means there is nothing to exchange, so this path is
     * testable without initialising the GPU. It must still select the row. */
    g_TrackTextureCursorRow = 7;
    g_TrackTexturePageWanted = 0;
    g_TrackTextureShadowPage[7] = 1;
    g_TrackTextureRowRect.y = 0;
    SwapTrackTextureRow();
    Check(g_TrackTextureRowRect.y == 0x107, "swap selects cursor row");
    Check(g_TrackTextureShadowPage[7] == 1,
          "nonrequested row is not toggled unnecessarily");
}

int main(void) {
    TrackPointWraps();
    DistanceSurvivesLargeSeparations();
    SecondTexturePageIsOneRange();
    TextureSwapStateResetsAndSkipsMatchingRows();
    if (s_failures != 0) {
        printf("%d track maths checks failed\n", s_failures);
        return 1;
    }
    printf("track index wrapping, planar distance and texture page all hold\n");
    return 0;
}
