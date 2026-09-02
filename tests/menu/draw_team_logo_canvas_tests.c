/*
 * The logo editor's screen, recorded.
 *
 * DrawTeamLogoCanvas is five sliding panels and about five hundred lines of
 * arithmetic on names like kreg, a0v and gx2, and nothing looked at it. The
 * visual reference frames in this suite never reach the logo screen, so the
 * only thing that would notice a change here is a person on the couch.
 *
 * A drawing function's behaviour is the sequence of drawing commands it
 * issues, so this stubs every primitive it can call, folds every argument of
 * every call into one number, and walks the states that gate the panels: how
 * far each one has slid, the zoom, palette mode, the guide, brush size, expert
 * mode, the colour channel and which pad is plugged in.
 */

#include "common.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render.h"
#include "game/render_state.h"
#include "game/state.h"
#include "game/team_logo.h"

#include <stdio.h>
#include <string.h>

/* The editor's own state. */
TeamLogoCanvas g_TeamLogoCanvas;
u16 g_TeamLogoClut[16];
u16 g_TeamLogoFadedClut[16];
u16 g_TeamLogoSwatches[16];
TeamLogoRect g_TeamLogoRect;
Rect g_TeamLogoClutRect;
u16 g_TeamLogoFadedClutRect;
TeamLogoColorIndex g_TeamLogoPenColor;
TeamLogoCoordinate g_TeamLogoCursorX;
TeamLogoCoordinate g_TeamLogoViewX;
s32 g_TeamLogoBrushSize;
s32 g_TeamLogoColorChannel;
s32 g_TeamLogoColorCycleAngle;
s32 g_TeamLogoCursorY;
s32 g_TeamLogoEditorStep;
u8 g_TeamLogoExpertMode;
s32 g_TeamLogoFadeLevel;
s32 g_TeamLogoGuideMode;
s32 g_TeamLogoPaletteMode;
s32 g_TeamLogoPanelStep;
s32 g_TeamLogoViewY;
s32 g_TeamLogoZoomLevel;
s32 g_TeamLogoZoomSpan;
u8 g_PadType;
GameRenderState g_RenderState;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;

static void Record(const char *name, const s32 *values, int count) {
    char line[512];
    const char *p;
    int used = snprintf(line, sizeof(line), "%s", name);
    int i;

    for (i = 0; i < count && used < (int)sizeof(line) - 16; i++) {
        used += snprintf(line + used, sizeof(line) - used, " %d", values[i]);
    }
    used += snprintf(line + used, sizeof(line) - used, "\n");
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (s_out != NULL) {
        fputs(line, s_out);
    }
    s_calls++;
}

#define RECORD(name, ...)                                                      \
    do {                                                                       \
        s32 v[] = {__VA_ARGS__};                                               \
        Record(name, v, (int)(sizeof(v) / sizeof(v[0])));                       \
    } while (0)

void DrawRectOutline(void *buf, s32 xa, s32 ya, s32 w, s32 h, u8 r, u8 g, u8 b,
                     u8 code) {
    (void)buf;
    RECORD("outline", xa, ya, w, h, r, g, b, code);
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 w, s32 h, s32 r, s32 g, s32 b,
                   s32 alpha) {
    (void)ot;
    RECORD("rect", x, y, w, h, r, g, b, alpha);
}

void DrawLine(GameOrderingTableEntry *ot, s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g, s32 b,
              s32 alpha) {
    (void)ot;
    RECORD("line", x0, y0, x1, y1, r, g, b, alpha);
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0, u8 r,
                u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    RECORD("sprite", x0, y0, x1, y1, u0, v0, r, g, b, clutX, shadeTex,
           semiTrans, (s32)flags);
}

void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage) {
    (void)ot;
    RECORD("quad", x0, y0, x1, y1, x2, y2, x3, y3, u0, v0, u1, v1, u2, v2, u3,
           v3, r, g, b, clutIndex, shadeTex, semiTrans, tpage);
}

s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 r, u8 g, u8 b,
                   u16 clut, u8 primitiveCount) {
    RECORD("number", x, y, flags, (s32)value, r, g, b, clut, primitiveCount);
    return 0;
}

void SetDrawClipRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 w, s32 h) {
    (void)ot;
    RECORD("clip", x, y, w, h);
}

/* The canvas transforms share this file with the drawer and play a cue; the
 * drawer itself never calls one. */
void PlaySoundCue(s32 cue) { (void)cue; }

/* LoadImage is a static inline shim in the port's GPU header, so it needs no
 * stub; the CLUT lookup is the console's own arithmetic. */
u_short GetClut(int x, int y) {
    RECORD("getclut", x, y);
    return (u_short)((y << 6) | ((x >> 4) & 0x3F));
}

/* The real sine table, so the colour cycle animates the way it does on the
 * console rather than being flattened to a constant. */
s32 rsin(s32 angle) {
    static const s32 quarter[17] = {0,    0x18F, 0x31F, 0x4AD, 0x63A, 0x7C4,
                                    0x94C, 0xACF, 0xC4E, 0xDC7, 0xF3A, 0x10A6,
                                    0x120A, 0x1365, 0x14B7, 0x15FF, 0x173C};
    s32 index = ((angle & 0xFFF) * 16) / 0x400;
    s32 sign = 1;

    if (index >= 32) {
        index -= 32;
        sign = -1;
    }
    if (index >= 16) {
        index = 32 - index;
    }
    return sign * quarter[index];
}

int main(int argc, char **argv) {
    /*
     * What the screen drew before the function was taken apart. Run the test
     * with a file name to write every drawing command out and diff two runs.
     */
    static const unsigned long expected = 4031224841UL;
    static const s32 panelSteps[] = {0, 0xA, 0x12, 0x19};
    static const s32 editorSteps[] = {0, 7, 8, 0x10};
    static const s32 zooms[] = {0, 0x100};
    static const s32 guides[] = {0, 1, 2};
    static const s32 brushes[] = {1, 4};
    static const s32 channels[] = {0, 1, 2};
    static const s32 pads[] = {0x23, 0x10};
    static const s32 args[] = {1, -1};
    s32 ot[64];
    int a, b, c, e, gi, bi, ci, pi, ai, x;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_TeamLogoRect.rect.x = 0x290;
    g_TeamLogoRect.rect.y = 0x30;
    g_TeamLogoRect.rect.w = 64;
    g_TeamLogoRect.rect.h = 16;
    g_TeamLogoClutRect.x = 16;
    g_TeamLogoClutRect.y = 480;
    g_TeamLogoClutRect.w = 16;
    g_TeamLogoClutRect.h = 1;

    for (a = 0; a < 4; a++)
    for (b = 0; b < 4; b++)
    for (c = 0; c < 2; c++)
    for (e = 0; e < 2; e++)
    for (gi = 0; gi < 3; gi++)
    for (bi = 0; bi < 2; bi++)
    for (ci = 0; ci < 3; ci++)
    for (pi = 0; pi < 2; pi++)
    for (ai = 0; ai < 2; ai++) {
        char label[128];

        for (x = 0; x < 64; x++) {
            g_TeamLogoCanvas.words[x][x & 7] = (u32)(0x12345678u + (u32)x);
        }
        for (x = 0; x < 16; x++) {
            g_TeamLogoClut[x] = (u16)(0x0421 * x);
            g_TeamLogoFadedClut[x] = 0;
            g_TeamLogoSwatches[x] = (u16)(0x1111 * x);
        }
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE_AS(void) = ot;

        g_TeamLogoPanelStep = panelSteps[a];
        g_TeamLogoEditorStep = editorSteps[b];
        g_TeamLogoZoomLevel = zooms[c];
        g_TeamLogoPaletteMode = e;
        g_TeamLogoGuideMode = guides[gi];
        g_TeamLogoBrushSize = brushes[bi];
        g_TeamLogoColorChannel = channels[ci];
        g_PadType = (u8)pads[pi];
        g_TeamLogoExpertMode = (u8)(ai == 0);
        g_TeamLogoColorCycleAngle = 0x321;
        g_TeamLogoFadeLevel = 0xC0;
        g_TeamLogoZoomSpan = 0x210;
        g_TeamLogoPenColor = (TeamLogoColorIndex)3;
        g_TeamLogoCursorX = (TeamLogoCoordinate)20;
        g_TeamLogoCursorY = 30;
        g_TeamLogoViewX = (TeamLogoCoordinate)4;
        g_TeamLogoViewY = 6;

        sprintf(label, "== p%d/e%d/z%d/m%d/g%d/b%d/c%d/pad%02x/arg%d",
                panelSteps[a], editorSteps[b], zooms[c], e, guides[gi],
                brushes[bi], channels[ci], pads[pi], args[ai]);
        Record(label, NULL, 0);

        DrawTeamLogoCanvas(args[ai], args[1 - ai]);

        {
            s32 after[6];
            after[0] = g_TeamLogoPanelStep;
            after[1] = g_TeamLogoEditorStep;
            after[2] = g_TeamLogoColorCycleAngle;
            after[3] = g_TeamLogoClut[0];
            after[4] = g_TeamLogoFadedClut[0];
            after[5] = g_TeamLogoFadedClut[15];
            Record("state", after, 6);
        }
        steps++;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL the logo screen draws differently: %d states issuing %d "
               "commands digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }

    g_TeamLogoFadeLevel = 0xF8;
    g_TeamLogoZoomLevel = 0xF8;
    RampTeamLogoCanvas(13, 21);
    if (g_TeamLogoFadeLevel != 0x100 || g_TeamLogoZoomLevel != 0x100 ||
        g_TeamLogoZoomSpan != 0x110) {
        puts("FAIL logo canvas upper ramp limit");
        return 1;
    }
    g_TeamLogoFadeLevel = 0x41;
    g_TeamLogoZoomLevel = 1;
    RampTeamLogoCanvas(-13, -21);
    if (g_TeamLogoFadeLevel != 0x40 || g_TeamLogoZoomLevel != 0 ||
        g_TeamLogoZoomSpan != 0x220) {
        puts("FAIL logo canvas lower ramp limit");
        return 1;
    }
    printf("the logo screen draws the same %d commands over %d states it "
           "always did\n", s_calls, steps);
    return 0;
}
