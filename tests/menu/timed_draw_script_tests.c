/*
 * The clock every menu animation runs on.
 *
 * RunTimedDrawScript is called from a hundred and thirty-six places across
 * twenty files, and it does two jobs in one body: it moves a script's
 * progress, and it draws whatever that progress has reached. Its return value
 * says whether the script has run out, which is what every menu screen gates
 * its input on, so a screen cannot be stepped without drawing it and drawing
 * cannot be checked without a renderer.
 *
 * Nothing tested it. This does, before it is taken apart: a synthetic script
 * covering every command type it knows, swept across the progress values and
 * the steps it is driven with, with every draw it makes and every value it
 * leaves behind folded into one number.
 *
 * The order is the part worth being careful about. A negative step rewinds
 * first and then draws; a positive one draws and then advances. So what is
 * drawn is never what the progress ends up as.
 */

#include "common.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/scratchpad.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

s32 g_MenuAltLayout;

static unsigned long s_digest = 2166136261UL;
static int s_calls;
static FILE *s_out;

static void Fold(long value) {
    unsigned long v = (unsigned long)value;
    int i;
    for (i = 0; i < 4; i++) {
        s_digest ^= (v >> (i * 8)) & 0xFF;
        s_digest = (s_digest * 16777619UL) & 0xFFFFFFFFUL;
    }
}

static void Record(const char *name, const s32 *values, int count) {
    const char *p;
    int i;
    for (p = name; *p != '\0'; p++) Fold((long)*p);
    for (i = 0; i < count; i++) Fold(values[i]);
    if (s_out != NULL) {
        fputs(name, s_out);
        for (i = 0; i < count; i++) fprintf(s_out, " %d", values[i]);
        fputc('\n', s_out);
    }
}

#define RECORD(name, ...) do {                                                \
    s32 v[] = {__VA_ARGS__};                                                  \
    Record(name, v, (int)(sizeof(v) / sizeof(v[0])));                         \
    s_calls++;                                                                \
} while (0)

/*
 * The real drawing boundary. The four DrawScripted* entry points live in the
 * same file as the walker, so they run for real and their interpolation is
 * under test too; what the module finally asks the renderer for is recorded
 * here.
 */
/*
 * The rest of the module draws menu furniture the script walker never
 * reaches. It shares a file with the walker, so it is stood up here rather
 * than pulled in; none of it is called by anything below.
 */
s32 g_AnimTimer;
s32 g_MenuCursorPulsePhase;
s32 g_MenuRowFlashLevels[16];
TimedDrawCommand g_MenuRowScript[1];
GameScratchpadRenderState g_RageScratchpadState;
void DrawFlatTriangle(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    RECORD("drawflattriangle", x0, y0, x1, (s32)y1, (s32)x2, (s32)y2, r, g, b, semiTrans, (s32)flags);
}
void DrawLargeText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut,
                   s32 flags) {
}
void DrawLine(
    void *ot,
    s32 x0,
    s32 y0,
    s32 x1,
    s32 y1,
    s32 r,
    s32 g,
    s32 b,
    s32 alpha) {
    RECORD("drawline", x0, y0, x1, y1, r, g, b, alpha);
}
void DrawRectOutline(void *buf, s32 xa, s32 ya, s32 w, s32 h, u8 r, u8 g,
                     u8 b, u8 code) {
}
void DrawSmallText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut, s32 flags) {
}
void DrawSolidRect(
    void *ot,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 r,
    s32 g,
    s32 b,
    s32 alpha) {
    RECORD("drawsolidrect", x, y, w, h, r, g, b, alpha);
}
void DrawSprite(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0,
                u8 r, u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans,
                u32 flags) {
    RECORD("drawsprite", x0, y0, x1, (s32)y1, (s32)u0, (s32)v0, r, g, b, (s32)clutX, shadeTex, semiTrans, (s32)flags);
}
void GameDrawTexturedQuad(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage) {
    RECORD("gamedrawtexturedquad", x0, y0, x1, (s32)y1, (s32)x2, (s32)y2, (s32)x3, (s32)y3, u0, v0, u1, v1, r, g, b, (s32)clutIndex, shadeTex, semiTrans, (s32)tpage);
}

s32 RunTimedDrawScript(void *commands, s32 *progress, s32 step);

/*
 * One command of every type the script walker knows, including the four that
 * are skipped when the alternate layout is on, one it ignores outright and
 * one past the range it looks at. Their times are spread so a given progress
 * has reached some and not others.
 */
#define COMMAND_COUNT 12
static TimedDrawCommand s_script[COMMAND_COUNT + 1];

/* Zeroed shapes and motions: the interpolation is deterministic, and what
 * matters here is which command ran and with how much time behind it. */
static ScriptedSpriteShape s_spriteShapes[COMMAND_COUNT];
static ScriptedSpriteMotion s_spriteMotions[COMMAND_COUNT];
static ScriptedLineShape s_lineShapes[COMMAND_COUNT];
static ScriptedLineMotion s_lineMotions[COMMAND_COUNT];
static ScriptedTriangleShape s_triShapes[COMMAND_COUNT];
static ScriptedTriangleMotion s_triMotions[COMMAND_COUNT];
static ScriptedQuadShape s_quadShapes[COMMAND_COUNT];
static ScriptedQuadMotion s_quadMotions[COMMAND_COUNT];

static void BuildScript(s32 limit) {
    static const s16 types[COMMAND_COUNT] = {0, 1, 9, 10, 19, 20, 29, 30, 39,
                                             5, 41, 0};
    static const s16 times[COMMAND_COUNT] = {0, 0, 4, 8, 8, 12, 16, 20, 24,
                                             4, 4, 32};
    int i;
    memset(s_script, 0, sizeof(s_script));
    memset(s_spriteShapes, 0, sizeof(s_spriteShapes));
    memset(s_spriteMotions, 0, sizeof(s_spriteMotions));
    memset(s_lineShapes, 0, sizeof(s_lineShapes));
    memset(s_lineMotions, 0, sizeof(s_lineMotions));
    memset(s_triShapes, 0, sizeof(s_triShapes));
    memset(s_triMotions, 0, sizeof(s_triMotions));
    memset(s_quadShapes, 0, sizeof(s_quadShapes));
    memset(s_quadMotions, 0, sizeof(s_quadMotions));
    for (i = 0; i < COMMAND_COUNT; i++) {
        s_script[i].time = times[i];
        s_script[i].type = types[i];
        switch (types[i]) {
        case 10: case 19:
            s_script[i].shape.lineShape = &s_lineShapes[i];
            s_script[i].motion.lineMotion = &s_lineMotions[i];
            break;
        case 20: case 29:
            s_script[i].shape.triangleShape = &s_triShapes[i];
            s_script[i].motion.triangleMotion = &s_triMotions[i];
            break;
        case 30: case 39:
            s_script[i].shape.quadShape = &s_quadShapes[i];
            s_script[i].motion.quadMotion = &s_quadMotions[i];
            break;
        default:
            s_script[i].shape.spriteShape = &s_spriteShapes[i];
            s_script[i].motion.spriteMotion = &s_spriteMotions[i];
            break;
        }
    }
    /* The walker stops on a negative time and reads the script's end out of
     * the terminator's second word. */
    s_script[COMMAND_COUNT].time = -1;
    s_script[COMMAND_COUNT].type = 0;
    s_script[COMMAND_COUNT].motion.value = limit;
}

int main(void) {
    /* Two with a step of one is the rewind landing exactly on one, which is
     * the boundary the floor at zero is written around. */
    static const s32 progresses[] = {-4, 0, 1, 2, 4, 8, 16, 24, 31, 32, 40};
    static const s32 steps[] = {-8, -1, 0, 1, 3, 8, 40};
    static const s32 limits[] = {0, 1, 32, 33};
    static const unsigned long expected = 2945563909UL;
    int pi, si, li, alt;
    int states = 0;

    s_out = getenv("RAGE_SCRIPT_TRACE") != NULL
                ? fopen(getenv("RAGE_SCRIPT_TRACE"), "w")
                : NULL;

    for (pi = 0; pi < (int)(sizeof(progresses) / sizeof(progresses[0])); pi++)
    for (si = 0; si < (int)(sizeof(steps) / sizeof(steps[0])); si++)
    for (li = 0; li < (int)(sizeof(limits) / sizeof(limits[0])); li++)
    for (alt = 0; alt < 2; alt++) {
        char label[128];
        s32 progress = progresses[pi];
        s32 result;

        BuildScript(limits[li]);
        g_MenuAltLayout = alt;
        sprintf(label, "== progress%d/step%d/limit%d/alt%d", progresses[pi],
                steps[si], limits[li], alt);
        Record(label, NULL, 0);
        result = RunTimedDrawScript(s_script, &progress, steps[si]);
        RECORD("after", result, progress);
        states++;
    }

    /*
     * The clock on its own. This is what the split was for: a screen's
     * animation can be stepped, and asked whether it has finished, without a
     * renderer anywhere near it. Nothing below draws.
     */
    {
        static const struct {
            const char *what;
            s32 progress;
            s32 step;
            s32 limit;
            s32 drawAt;
            s32 after;
            s32 finished;
        } cases[] = {
            {"a rest step neither moves nor finishes", 8, 0, 32, 8, 8, 0},
            {"advancing short of the end", 8, 3, 32, 8, 11, 0},
            {"advancing onto the end finishes", 29, 3, 32, 29, 32, 1},
            {"advancing past the end stops at it", 8, 40, 32, 8, 32, 1},
            {"a script with no length finishes at once", 0, 1, 0, 0, 0, 1},
            {"already at the end finishes again", 32, 1, 32, 32, 32, 1},
            /* A rewind never reports finished, whatever it lands on. */
            {"rewinding", 8, -3, 32, 5, 5, 0},
            {"rewinding to the start", 3, -3, 32, 0, 0, 0},
            {"rewinding past the start stops there", 3, -8, 32, 0, 0, 0},
            {"rewinding onto one keeps it", 2, -1, 32, 1, 1, 0},
            /* What is drawn is where the clock stood, not where it ends. */
            {"drawing happens before the advance", 0, 8, 32, 0, 8, 0},
        };
        size_t ci;
        int clockFailures = 0;

        for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++) {
            s32 progress = cases[ci].progress;
            TimedDrawScriptTick tick;
            s32 callsBefore = s_calls;
            BuildScript(cases[ci].limit);
            tick = AdvanceTimedDrawScript(s_script, &progress, cases[ci].step);
            if (tick.drawAt != cases[ci].drawAt) {
                printf("FAIL %s: draws at %d, expected %d\n", cases[ci].what,
                       tick.drawAt, cases[ci].drawAt);
                clockFailures++;
            }
            if (progress != cases[ci].after) {
                printf("FAIL %s: left the clock at %d, expected %d\n",
                       cases[ci].what, progress, cases[ci].after);
                clockFailures++;
            }
            if (tick.finished != cases[ci].finished) {
                printf("FAIL %s: finished %d, expected %d\n", cases[ci].what,
                       tick.finished, cases[ci].finished);
                clockFailures++;
            }
            if (s_calls != callsBefore) {
                printf("FAIL %s: moving the clock drew something\n",
                       cases[ci].what);
                clockFailures++;
            }
        }
        if (clockFailures != 0) {
            printf("%d script clock assertion(s) failed\n", clockFailures);
            return 1;
        }
    }

    if (s_out != NULL) fclose(s_out);
    if (s_digest != expected) {
        printf("FAIL the script walker behaves differently: %d states making "
               "%d calls digest to %lu, expected %lu\n", states, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("the script walker takes the same %d states it always did\n",
           states);
    return 0;
}
