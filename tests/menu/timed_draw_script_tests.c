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
#include "game/render_state.h"

#include <limits.h>
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
TimedDrawCommand g_MenuRowScript[4];
GameRenderState g_RenderState;
void DrawFlatTriangle(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags) {
    RECORD("drawflattriangle", x0, y0, x1, (s32)y1, (s32)x2, (s32)y2, r, g, b, semiTrans, (s32)flags);
}
void DrawLargeText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut,
                   s32 flags) {
}
void DrawLine(
    GameOrderingTableEntry *ot,
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
void DrawRectOutline(GameOrderingTableEntry *buf, s32 xa, s32 ya, s32 w,
                     s32 h, u8 r, u8 g,
                     u8 b, u8 code) {
}
void DrawSmallText(s32 x0, s16 y, const char *str0, u8 color, u8 g, u8 b,
                   u16 clut, s32 flags) {
}
void DrawSolidRect(
    GameOrderingTableEntry *ot,
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
void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0,
                u8 r, u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans,
                u32 flags) {
    RECORD("drawsprite", x0, y0, x1, (s32)y1, (s32)u0, (s32)v0, r, g, b, (s32)clutX, shadeTex, semiTrans, (s32)flags);
}
void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage) {
    RECORD("gamedrawtexturedquad", x0, y0, x1, (s32)y1, (s32)x2, (s32)y2, (s32)x3, (s32)y3, u0, v0, u1, v1, r, g, b, (s32)clutIndex, shadeTex, semiTrans, (s32)tpage);
}

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step);

/*
 * One command of every type the script walker knows, including the four that
 * are skipped when the alternate layout is on, one it ignores outright and
 * one past the range it looks at. Their times are spread so a given progress
 * has reached some and not others.
 */
#define COMMAND_COUNT 12
static TimedDrawCommand s_script[COMMAND_COUNT + 1];

/* Each primitive gets real geometry and both signs of packed velocity. This
 * makes the sweep cover interpolation as well as command dispatch. */
static ScriptedSpriteShape s_spriteShapes[COMMAND_COUNT];
static ScriptedSpriteMotion s_spriteMotions[COMMAND_COUNT];
static ScriptedLineShape s_lineShapes[COMMAND_COUNT];
static ScriptedLineMotion s_lineMotions[COMMAND_COUNT];
static ScriptedTriangleShape s_triShapes[COMMAND_COUNT];
static ScriptedTriangleMotion s_triMotions[COMMAND_COUNT];
static ScriptedQuadShape s_quadShapes[COMMAND_COUNT];
static ScriptedQuadMotion s_quadMotions[COMMAND_COUNT];

static s32 PackVelocity(s16 x, s16 y) {
    return (s32)((u32)(u16)x | ((u32)(u16)y << 16));
}

static int TestFadingMenuSprites(void) {
    ScriptedSpriteShape shapes[3];
    ScriptedSpriteMotion motions[3];
    int callsBefore;
    int i;

    memset(shapes, 0, sizeof(shapes));
    memset(motions, 0, sizeof(motions));
    memset(g_MenuRowScript, 0, sizeof(g_MenuRowScript));
    memset(g_MenuRowFlashLevels, 0, sizeof(g_MenuRowFlashLevels));
    for (i = 0; i < 3; i++) {
        shapes[i].width = (s16)(20 + i);
        shapes[i].height = (s16)(10 + i);
        shapes[i].u = (u8)(2 + i);
        shapes[i].v = (u8)(4 + i);
        shapes[i].alpha = (u8)(0x50 + i);
        motions[i].x = (s16)(100 + i * 10);
        motions[i].y = (s16)(50 + i * 5);
        motions[i].clut = (u16)(0x180 + i);
        g_MenuRowScript[i].shape.spriteShape = &shapes[i];
        g_MenuRowScript[i].motion.spriteMotion = &motions[i];
    }
    g_MenuRowScript[0].time = 4;
    motions[0].limit = 16;
    motions[0].packedVelocity = PackVelocity(-32, 64);

    g_MenuRowFlashLevels[1] = 77;
    callsBefore = s_calls;
    DrawFadingMenuSprites(3, 2, 1);
    if (s_calls != callsBefore || g_MenuRowFlashLevels[1] != 77) {
        puts("FAIL fading rows draw before their start");
        return 0;
    }

    g_MenuRowFlashLevels[0] = 59;
    g_MenuRowFlashLevels[1] = 77;
    g_MenuRowFlashLevels[2] = 120;
    DrawFadingMenuSprites(12, 2, 1);
    if (s_calls != callsBefore + 3 || g_MenuRowFlashLevels[0] != 0 ||
        g_MenuRowFlashLevels[1] != 448 || g_MenuRowFlashLevels[2] != 60) {
        puts("FAIL fading rows draw range or timers");
        return 0;
    }

    callsBefore = s_calls;
    DrawFadingMenuSprites(12, -1, 1);
    if (s_calls != callsBefore) {
        puts("FAIL fading rows accept a negative last row");
        return 0;
    }
    DrawFadingMenuSprites(12, FADING_MENU_ROW_COUNT, 1);
    DrawFadingMenuSprites(12, 2, FADING_MENU_ROW_COUNT);
    if (s_calls != callsBefore) {
        puts("FAIL fading rows accept an out-of-range row");
        return 0;
    }
    return 1;
}

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
        s16 vx = (i & 1) ? (s16)(-17 - i) : (s16)(11 + i);
        s16 vy = (i & 2) ? (s16)(-9 - i) : (s16)(7 + i);

        s_spriteShapes[i].width = (s16)(12 + i);
        s_spriteShapes[i].height = (s16)(8 + i);
        s_spriteShapes[i].u = (u8)(3 + i);
        s_spriteShapes[i].v = (u8)(5 + i);
        s_spriteShapes[i].flags = (u8)(i & 0xF);
        s_spriteShapes[i].alpha = (u8)(0x40 + i);
        s_spriteMotions[i].limit = 19 + i;
        s_spriteMotions[i].x = (s16)(40 + i);
        s_spriteMotions[i].y = (s16)(70 - i);
        s_spriteMotions[i].clut = (u16)(0x120 + i);
        s_spriteMotions[i].r = (u8)(20 + i);
        s_spriteMotions[i].g = (u8)(40 + i);
        s_spriteMotions[i].b = (u8)(60 + i);
        s_spriteMotions[i].packedVelocity = PackVelocity(vx, vy);

        s_lineShapes[i].r = (u8)(30 + i);
        s_lineShapes[i].g = (u8)(50 + i);
        s_lineShapes[i].b = (u8)(70 + i);
        s_lineShapes[i].flags = (u8)(i & 7);
        s_lineMotions[i].limit = 17 + i;
        s_lineMotions[i].x0 = (s16)(10 + i);
        s_lineMotions[i].y0 = (s16)(20 + i);
        s_lineMotions[i].x1 = (s16)(80 - i);
        s_lineMotions[i].y1 = (s16)(90 - i);
        s_lineMotions[i].packedVelocity0 = PackVelocity(vx, vy);
        s_lineMotions[i].packedVelocity1 = PackVelocity((s16)-vy, (s16)-vx);

        s_triShapes[i].x1 = (u16)(15 + i);
        s_triShapes[i].y1 = (u16)(5 + i);
        s_triShapes[i].x2 = (u16)(7 + i);
        s_triShapes[i].y2 = (u16)(18 + i);
        s_triShapes[i].r = (u8)(80 + i);
        s_triShapes[i].g = (u8)(90 + i);
        s_triShapes[i].b = (u8)(100 + i);
        s_triShapes[i].flags = (u8)(i & 7);
        s_triMotions[i].limit = 23 + i;
        s_triMotions[i].x = (s16)(100 + i);
        s_triMotions[i].y = (s16)(50 - i);
        s_triMotions[i].packedVelocity = PackVelocity(vx, vy);

        s_quadShapes[i].u0 = (u8)i;
        s_quadShapes[i].v0 = (u8)(i + 1);
        s_quadShapes[i].u1 = (u8)(i + 2);
        s_quadShapes[i].v1 = (u8)(i + 3);
        s_quadShapes[i].u2 = (u8)(i + 4);
        s_quadShapes[i].v2 = (u8)(i + 5);
        s_quadShapes[i].u3 = (u8)(i + 6);
        s_quadShapes[i].v3 = (u8)(i + 7);
        s_quadShapes[i].clut = (u16)(0x220 + i);
        s_quadShapes[i].r = (u8)(110 + i);
        s_quadShapes[i].g = (u8)(120 + i);
        s_quadShapes[i].b = (u8)(130 + i);
        s_quadShapes[i].flags = (u8)(i & 0xF);
        s_quadShapes[i].alpha = (u8)(0x60 + i);
        s_quadMotions[i].limit = 29 + i;
        s_quadMotions[i].x = (s16)(30 + i);
        s_quadMotions[i].y = (s16)(35 + i);
        s_quadMotions[i].width = (s16)(45 + i);
        s_quadMotions[i].height = (s16)(25 + i);
        s_quadMotions[i].packedVelocity = PackVelocity(vx, vy);
        s_quadMotions[i].packedSizeVelocity =
            PackVelocity((s16)-vy, (s16)-vx);

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
    static const unsigned long expected = 1344526518UL;
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
            {"large advance saturates at the end", INT_MAX, 1, INT_MAX,
             INT_MAX, INT_MAX, 1},
            {"large rewind saturates at the start", INT_MIN, -1, 32,
             0, 0, 0},
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

    if (!TestFadingMenuSprites()) return 1;

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
