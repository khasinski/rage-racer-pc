/*
 * Where the race HUD's sprites are built.
 *
 * The widescreen layout pushes the HUD out to the edges of the picture, and
 * DrawRaceHud recomputes that each frame for the labels it draws itself. Two
 * of them it does not draw: the split time and the race position take the
 * sprite this builder made and change only its texture coordinates, so an
 * unanchored build left those two sitting where a 4:3 screen put them while
 * everything around them had moved.
 *
 * So this asks the builder, not the drawing, and asks it about every sprite
 * rather than the ones a particular frame happens to redo.
 */

#include "common.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "port_config.h"
#include "rage/hud_config.h"

#include <stdio.h>
#include <string.h>

void BuildRaceHudPrims(s32 mode);

static RagePortConfig s_config;
static int s_modernEnabled = 1;
static const char *s_anchor = "edges";
static int s_failures;

const RagePortConfig *PortActiveConfig(void) { return &s_config; }
int ModernIsEnabled(void) { return s_modernEnabled; }
const char *RuntimeConfigGet(const char *key) {
    if (!strcmp(key, "hud.anchor")) return s_anchor;
    return NULL;
}
int RuntimeConfigEnabled(const char *key) {
    const char *value = RuntimeConfigGet(key);
    return value != NULL && strcmp(value, "false") != 0;
}

/* The game's own state the builder reaches for. */
GameFrameContext g_FrameContexts[2];
s32 g_GrandPrixClass;
GameSpriteDesc g_TachoNeedleSprite;
GameSpriteDesc g_RaceHudSpriteDescsGp[12];
GameSpriteDesc g_RaceHudSpriteDescsTimeTrial[11];

/* Building one sprite from a description is somebody else's business; what
 * matters here is the position the builder puts on it afterwards. */
void BuildSpriteFromDesc(SPRT *sprite, GameSpriteDesc *desc) {
    memset(sprite, 0, sizeof(*sprite));
    sprite->x0 = (s16)desc->x;
    sprite->y0 = (s16)desc->y;
    sprite->u0 = desc->u0;
    sprite->v0 = desc->v0;
}
/* SetDrawMode and SetShadeTex are inline in the PSY-Q headers, so they run
 * for real; neither touches a sprite's position. */

static void Expect(const char *what, int row, int got, int want) {
    if (got == want) return;
    printf("FAIL %s row %d: x is %d, expected %d\n", what, row, got, want);
    s_failures++;
}

/*
 * Positions covering the three cases the layout has to tell apart: hard
 * against the left, hard against the right, and out in the middle, which is
 * where the split time's sign lives at 120 beside a time drawn at 128.
 */
static void LayOutDescs(void) {
    static const u16 xs[12] = {240, 240, 240, 240, 240, 240,
                               244, 8, 244, 8, 120, 54};
    int i;
    for (i = 0; i < 12; i++) {
        g_RaceHudSpriteDescsGp[i].x = xs[i];
        g_RaceHudSpriteDescsGp[i].y = (u16)(16 + i);
    }
    for (i = 0; i < 11; i++) {
        g_RaceHudSpriteDescsTimeTrial[i].x = xs[i];
        g_RaceHudSpriteDescsTimeTrial[i].y = (u16)(24 + i);
    }
}

static void CheckMode(const char *what, s32 mode, int rows,
                      const GameSpriteDesc *descs) {
    int row;
    BuildRaceHudPrims(mode);
    for (row = 0; row < rows; row++) {
        int x = descs[row].x;
        int want = HudAnchorX(x);
        SPRT *sprite = row < 6
            ? &g_FrameContexts[0].layout.raceHud.lapTimes[row]
            : &g_FrameContexts[0].layout.raceHud.labels[row - 6];
        Expect(what, row, sprite->x0, want);
        /* Both frame contexts are built, and identically. */
        sprite = row < 6
            ? &g_FrameContexts[1].layout.raceHud.lapTimes[row]
            : &g_FrameContexts[1].layout.raceHud.labels[row - 6];
        Expect(what, row, sprite->x0, want);
    }
}

int main(void) {
    LayOutDescs();
    s_config.modernAspect = RAGE_MODERN_ASPECT_16_9;

    /* Time attack builds eleven, and five of them are labels. Rows nine and
     * ten are the two DrawRaceHud never repositions. */
    CheckMode("time attack, anchored", 0, 11, g_RaceHudSpriteDescsTimeTrial);
    CheckMode("grand prix, anchored", 1, 12, g_RaceHudSpriteDescsGp);

    /* The three cases, spelled out rather than left to the loop above. */
    Expect("left edge follows the left", 0, HudAnchorX(8), HudLeftX(8));
    Expect("right edge follows the right", 0, HudAnchorX(244),
           HudRightX(244));
    Expect("the middle stays where it is", 0, HudAnchorX(120), 120);
    Expect("and so does the centre line", 0, HudAnchorX(160), 160);

    /* Whether the margin applies at all is hud_config's own test; the
     * configuration is read once and cached, so it cannot be moved here. */

    if (s_failures != 0) {
        printf("%d race HUD layout assertion(s) failed\n", s_failures);
        return 1;
    }
    printf("every race HUD sprite is built where the layout puts it\n");
    return 0;
}
