/*
 * The course select screen, swept.
 *
 * The screen that starts a race: it picks the course, offers to save, and is
 * the only place the Grand Prix class can be changed, which resets the
 * player's progress through the series. Three hundred and eighty lines nested
 * seven deep across ten states, with a goto that jumps out of one case of a
 * switch into the middle of another, and nothing tested it.
 *
 * So this walks the states that decide which branch runs and folds everything
 * the call could have written, plus every outward call it made, into one
 * number.
 */

#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_CarNamePlateStep;
s32 g_CarSwapFromIndex;
s32 g_CarSwapToIndex;
s32 g_ClassChangeApplied;
s32 g_CourseCardPendingGrade;
s32 g_CourseCardSpin;
s32 g_CourseCardSpinTarget;
s32 g_CourseIndex;
CourseProgressState *g_CourseProgress;
u8 g_CourseSelectGpScript;
u8 *g_CourseSelectModalScript;
s32 g_CourseSelectOption;
/* The prompts are decoded command arrays; never walked here, only named. */
TimedDrawCommand g_CourseSelectSavePromptBanner[2];
TimedDrawCommand g_CourseSelectSavePromptScript[4];
TimedDrawCommand g_MenuDialogPanelLowerScript[8];
u8 g_CourseSelectTimeAttackScript;
s32 g_CourseSwapDelay;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
u8 g_MenuBlankCaption;
s32 g_MenuConfirmTimer;
s32 g_MenuCourseModelIndex;
s32 g_MenuHandlerIndex;
s32 g_MenuHandlerIndex2;
s32 g_MenuHintBarStep;
s32 g_MenuOutgoingScreenProgress;
s32 g_MenuOverlayPattern;
s32 g_MenuPendingCourseIndex;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
u8 g_MenuSubCursor;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s32 g_MenuViewOffset;
s32 g_MenuViewOffsetTarget;
volatile u16 g_PadHeld;
u16 g_PadPressed;
s32 g_PlayerCarIndex;
s32 g_PlayerMoney;
GameRaceProgress *g_RaceProgress;
s32 g_SceneId;
s32 g_TimeAttackPlateStep;
u8 g_UiChromeScript;
u8 g_UiChromeScript2;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;
GameScratchpadRenderState g_RageScratchpadState;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;
static s32 s_scriptResult;
static s32 s_curtain;
static s32 s_canPrev;
static s32 s_canNext;

static void Fold(unsigned char byte) {
    s_digest = ((s_digest ^ byte) * 16777619UL) & 0xFFFFFFFFUL;
}

/* The digest folds raw values rather than their text; the readable form is
 * only produced when a file was asked for. */
static void Record(const char *name, const s32 *values, int count) {
    const char *p;
    int i;

    for (p = name; *p != '\0'; p++) {
        Fold((unsigned char)*p);
    }
    for (i = 0; i < count; i++) {
        u32 value = (u32)values[i];

        Fold((unsigned char)value);
        Fold((unsigned char)(value >> 8));
        Fold((unsigned char)(value >> 16));
        Fold((unsigned char)(value >> 24));
    }
    if (s_out != NULL) {
        fputs(name, s_out);
        for (i = 0; i < count; i++) {
            fprintf(s_out, " %d", values[i]);
        }
        fputc('\n', s_out);
    }
    s_calls++;
}

#define RECORD(name, ...)                                                      \
    do {                                                                       \
        s32 v[] = {__VA_ARGS__};                                               \
        Record(name, v, (int)(sizeof(v) / sizeof(v[0])));                       \
    } while (0)

static s32 ScriptId(const void *commands) {
    if (commands == &g_CourseSelectGpScript) return 1;
    if (commands == &g_CourseSelectTimeAttackScript) return 2;
    if (commands == &g_UiChromeScript) return 3;
    if (commands == &g_UiChromeScript2) return 4;
    if (commands == g_CourseSelectSavePromptScript) return 5;
    if (commands == g_CourseSelectSavePromptBanner) return 6;
    if (commands == g_MenuDialogPanelLowerScript) return 7;
    return commands == NULL ? 0 : 8;
}

s32 RunTimedDrawScript(void *commands, s32 *progress, s32 step) {
    RECORD("script", ScriptId(commands), progress == &g_UiScriptProgress2,
           step);
    return s_scriptResult;
}

void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    RECORD("nameplate", step, model, grade);
}
void DrawMenuCourseView(void) { RECORD("courseview", 0); }
void DrawMenuLightBurst(s32 arg) { RECORD("burst", arg); }
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight) {
    RECORD("arrows", step, wide, drawLeft, drawRight);
}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    RECORD("sprites", progress, count, slot);
}
void DrawOwnedCarCounter(s32 owned, s32 step) {
    RECORD("counter", owned, step);
}
void DrawMenuCursorBox(s32 x0, s32 y0, s32 x1, s32 y1, s32 flash) {
    RECORD("cursorbox", x0, y0, x1, y1, flash);
}
void DrawSprite(void *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0, u8 r,
                u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    RECORD("sprite", x0, y0, x1, y1, u0, v0, r, g, b, clutX, shadeTex,
           semiTrans, (s32)flags);
}
void GameDrawMenuButton(s32 x0, s32 y0, s32 x1, s32 y1, u8 r, u8 g, u8 b,
                        s32 flags, s32 textX, s32 textY, u8 *caption) {
    RECORD("button", x0, y0, x1, y1, r, g, b, flags, textX, textY,
           caption == &g_MenuBlankCaption);
}
void DrawTimeAttackPlate(s32 stepArg) { RECORD("timeattackplate", stepArg); }
void FlipCourseCard(s32 *p0, s32 *p1, s32 *p2) {
    RECORD("flipcard", *p0, *p1, *p2);
}
/* How far the curtain has drawn across is what the class change waits on, so
 * the sweep sets it rather than the stub deciding. */
s32 DrawClassChangeCurtain(s32 step) {
    RECORD("curtain", step);
    return s_curtain;
}
void ResetCourseProgress(s32 mode) { RECORD("resetprogress", mode); }
void StartSequenceFadeOut(void) { RECORD("fadeout", 0); }
void PlaySoundCue(s32 cue) { RECORD("cue", cue); }
/* Whether the course either side of this one may be picked lives in the file
 * the screen came out of; what matters here is that the screen asks. */
s32 CanSelectPrevCourse(void) { RECORD("canprev", 0); return s_canPrev; }
s32 CanSelectNextCourse(void) { RECORD("cannext", 0); return s_canNext; }

static GameRaceProgress s_progress;
static CourseProgressState s_course;

int main(int argc, char **argv) {
    /*
     * What the screen did before it was taken apart. Run the test with a file
     * name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 1588266003UL;
    static const s32 busyStates[] = {0, -1, -2, -3, -4, -5, 1, 2, 3, 4};
    static const u16 buttons[] = {0, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT,
                                  PAD_CONFIRM, PAD_CANCEL};
    static const s32 courses[] = {0, 4};
    /* The class change waits for the curtain to reach 0x19 and then to come
     * back to nothing, so the sweep sits on both. */
    static const s32 curtains[] = {0, 0x19};
    static const s32 offsets[] = {0x3D08F, 0x3D090};
    /* Class five is the extra series, which has no round of its own, so the
     * sweep sits either side of that as well as on it. */
    static const s32 classes[] = {2, 4, 5};
    s32 ot[64];
    int bi, sr, p2, gp, opt, pb, sub, cur, timer, prog, off, ci, applied, kl;
    int steps = 0;
    int promptFailures = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_RaceProgress = &s_progress;
    g_CourseProgress = &s_course;

    for (bi = 0; bi < 10; bi++)
    for (sr = 0; sr < 2; sr++)
    for (p2 = 0; p2 < 2; p2++)
    for (gp = 0; gp < 2; gp++)
    for (opt = 0; opt < 3; opt++)
    for (pb = 0; pb < 7; pb++)
    for (sub = 0; sub < 3; sub++)
    for (cur = 0; cur < 2; cur++)
    for (timer = 0; timer < 2; timer++)
    for (prog = 0; prog < 2; prog++)
    for (off = 0; off < 2; off++)
    for (ci = 0; ci < 2; ci++)
    /* Whether the curtain has already been applied decides which half of the
     * class change runs, so it cannot ride on the curtain's own axis. */
    for (applied = 0; applied < 2; applied++)
    for (kl = 0; kl < 3; kl++) {
        char label[224];
        int i;

        memset(&s_progress, 0, sizeof(s_progress));
        memset(&s_course, 0, sizeof(s_course));
        memset(ot, 0, sizeof(ot));
        SCRATCH_OT_BASE_AS(void) = ot;
        for (i = 0; i < 4; i++) {
            s_course.bestPlace[i] = (u8)(i + 1);
        }
        s_progress.maxClassReached = 2;

        GameMenuBusy = busyStates[bi];
        s_scriptResult = sr;
        s_curtain = curtains[cur];
        s_canPrev = ci;
        s_canNext = 1 - ci;
        g_UiScriptProgress2 = p2;
        g_UiScriptProgress = prog;
        g_GrandPrixMode = (s16)gp;
        g_CourseSelectOption = opt;
        g_PadPressed = buttons[pb];
        g_PadHeld = 0;
        g_MenuSubCursor = (u8)sub;
        g_MenuConfirmTimer = timer;
        g_MenuViewOffset = offsets[off];
        g_MenuOutgoingScreenProgress = off;
        g_ClassChangeApplied = applied;
        g_CourseIndex = courses[ci];

        g_MenuAltLayoutSetting = 1;
        g_CarNamePlateStep = 4;
        g_MenuPlateCarIndex = 2;
        g_CarSwapFromIndex = 0;
        g_CarSwapToIndex = 0;
        g_CourseCardPendingGrade = 0;
        g_CourseCardSpin = 0x1000;
        g_CourseCardSpinTarget = 0x800;
        g_CourseSwapDelay = 0;
        g_GrandPrixClass = classes[kl];
        g_GrandPrixSeries = 3;
        g_MenuCourseModelIndex = 0;
        g_MenuHandlerIndex = 0;
        g_MenuHandlerIndex2 = 0;
        g_MenuHintBarStep = 0;
        g_MenuOverlayPattern = 0;
        g_MenuPendingCourseIndex = 0;
        g_MenuScreen = 0;
        g_MenuViewAngle = 0;
        g_MenuViewAngleTarget = 0;
        g_MenuViewOffsetTarget = 0;
        g_PlayerCarIndex = 9;
        g_PlayerMoney = 12345;
        g_SceneId = 0;
        g_TimeAttackPlateStep = 0;
        g_CourseSelectModalScript = NULL;

        sprintf(label,
                "== busy%d/script%d/p2_%d/gp%d/opt%d/pad%04x/sub%d/curtain%d/"
                "timer%d/prog%d/off%d/course%d/applied%d/class%d",
                busyStates[bi], sr, p2, gp, opt, buttons[pb], sub,
                curtains[cur], timer, prog, off, courses[ci], applied,
                classes[kl]);
        Record(label, NULL, 0);

        UpdateCourseSelectScreen();

        {
            s32 after[23];
            after[0] = GameMenuBusy;
            after[1] = g_CourseSelectOption;
            after[2] = g_CourseIndex;
            after[3] = g_MenuSubCursor;
            after[4] = g_MenuConfirmTimer;
            after[5] = g_MenuScreen;
            after[6] = g_MenuHandlerIndex;
            after[7] = g_MenuHandlerIndex2;
            after[8] = g_MenuOverlayPattern;
            after[9] = g_MenuHintBarStep;
            after[10] = g_SceneId;
            after[11] = g_GrandPrixClass;
            after[12] = g_GrandPrixSeries;
            after[13] = g_ClassChangeApplied;
            after[14] = g_CourseCardSpin;
            after[15] = g_CourseCardPendingGrade;
            after[16] = g_TimeAttackPlateStep;
            after[17] = g_MenuViewAngle;
            after[18] = g_MenuViewOffset;
            after[19] = g_MenuViewOffsetTarget;
            after[20] = g_MenuPendingCourseIndex;
            after[21] = ScriptId(g_CourseSelectModalScript);
            /* The showroom angle the class change swings to. It sits beside
             * the offset above and was the one thing the class change writes
             * that nothing here looked at. */
            after[22] = g_MenuViewAngleTarget;
            Record("state", after, 23);
            RECORD("saved", s_progress.course, s_progress.carIndex,
                   s_progress.classIndex, s_progress.money.value,
                   g_UiScriptProgress, g_MenuCourseModelIndex,
                   g_CarSwapFromIndex, g_CarSwapToIndex);
        }
        steps++;
    }

    /*
     * Browsing left and right is only reachable while the screen is idle and
     * the pad is held, which the sweep above never is, so it gets a pass of
     * its own: both directions and both at once, either side of the distance
     * the card has to have settled within, and with the swap already pending
     * or not.
     */
    {
        static const u16 held[] = {0, PAD_LEFT, PAD_RIGHT, PAD_LEFT | PAD_RIGHT};
        static const s32 settleOffsets[] = {0, 0x3D08F, 0x3D090};
        /* Zero is a pending index that is neither "none" nor a real course. */
        static const s32 pendings[] = {-1, 0, 2};
        /* Stepping either way off course three lands on the boundary between
         * the time attack plate being shown and hidden. */
        static const s32 browseCourses[] = {0, 3, 4};
        int hb, se, pend, allow, gpi, cj;

        for (hb = 0; hb < 4; hb++)
        for (se = 0; se < 3; se++)
        for (pend = 0; pend < 3; pend++)
        for (allow = 0; allow < 2; allow++)
        for (gpi = 0; gpi < 2; gpi++)
        for (cj = 0; cj < 3; cj++) {
            char label[160];
            int i;

            memset(&s_progress, 0, sizeof(s_progress));
            memset(&s_course, 0, sizeof(s_course));
            memset(ot, 0, sizeof(ot));
            SCRATCH_OT_BASE_AS(void) = ot;
            for (i = 0; i < 4; i++) {
                s_course.bestPlace[i] = (u8)(i + 1);
            }
            s_progress.maxClassReached = 2;

            GameMenuBusy = 0;
            s_scriptResult = 1;
            s_curtain = 0;
            s_canPrev = allow;
            s_canNext = 1 - allow;
            g_UiScriptProgress2 = 0;
            g_UiScriptProgress = 0;
            g_GrandPrixMode = (s16)gpi;
            g_GrandPrixClass = 2;
            g_CourseSelectOption = 0;
            g_PadPressed = 0;
            g_PadHeld = held[hb];
            g_MenuViewAngleTarget = 0x7A120;
            g_MenuViewAngle = 0x7A120 + settleOffsets[se];
            g_MenuPendingCourseIndex = pendings[pend];
            g_CourseIndex = browseCourses[cj];
            g_CourseCardSpin = 0x1000;
            g_CourseCardSpinTarget = 0x800;
            g_CourseSwapDelay = 7;
            g_MenuCourseModelIndex = 0;
            g_TimeAttackPlateStep = 0;
            g_CourseCardPendingGrade = 0;
            g_CourseSelectModalScript = NULL;
            g_MenuSubCursor = 0;
            g_MenuConfirmTimer = 0;
            g_MenuAltLayoutSetting = 1;

            sprintf(label, "== browse held%04x/settle%d/pending%d/allow%d/"
                    "gp%d/course%d", held[hb], settleOffsets[se],
                    pendings[pend], allow, gpi, browseCourses[cj]);
            Record(label, NULL, 0);
            UpdateCourseSelectScreen();
            RECORD("browsed", g_CourseIndex, g_MenuPendingCourseIndex,
                   g_MenuCourseModelIndex, g_MenuViewAngle,
                   g_MenuViewAngleTarget, g_CourseCardSpin,
                   g_CourseCardPendingGrade, g_TimeAttackPlateStep,
                   g_CourseSwapDelay);
            steps++;
        }
    }

    /*
     * Handing the screen over. Every chosen destination waits for the
     * showroom to have swung clear, and the race waits for the outgoing
     * animation as well. The sweep above drives the offset and that animation
     * from one index, so its two settings move together and the one
     * combination that actually hands over, a finished animation with the
     * showroom clear, never came up. Both are held apart here.
     */
    {
        static const s32 chosen[] = {1, 2, 3, 4};
        static const s32 offs[] = {0x3D08E, 0x3D08F, 0x3D090};
        static const s32 courseIn[] = {0, 5};
        int ch, ofi, prog, cj;

        for (ch = 0; ch < 4; ch++)
        for (ofi = 0; ofi < 3; ofi++)
        for (prog = 0; prog < 2; prog++)
        for (cj = 0; cj < 2; cj++) {
            char label[160];

            memset(&s_progress, 0, sizeof(s_progress));
            memset(&s_course, 0, sizeof(s_course));
            memset(ot, 0, sizeof(ot));
            SCRATCH_OT_BASE_AS(void) = ot;
            s_progress.maxClassReached = 2;

            GameMenuBusy = chosen[ch];
            s_scriptResult = 1;
            s_curtain = 0;
            s_canPrev = 1;
            s_canNext = 1;
            /* The outgoing animation has run out, which is the only way
             * EnterChosenScreen is reached at all. */
            g_UiScriptProgress = 0;
            g_UiScriptProgress2 = 0;
            g_GrandPrixMode = 1;
            g_CourseSelectOption = 0;
            g_PadPressed = 0;
            g_PadHeld = 0;
            g_MenuSubCursor = 0;
            g_MenuConfirmTimer = 0;
            g_MenuViewOffset = offs[ofi];
            g_MenuOutgoingScreenProgress = prog;
            g_ClassChangeApplied = 0;
            /* Five is the one of these the handover's mask changes, so the
             * course it writes is not the course it was given. */
            g_CourseIndex = courseIn[cj];
            g_SceneId = -1;
            g_PlayerCarIndex = 3;
            g_GrandPrixClass = 1;
            g_PlayerMoney = 4321;
            g_GrandPrixSeries = 7;
            g_MenuAltLayoutSetting = 1;
            g_CarNamePlateStep = 4;
            g_MenuPlateCarIndex = 2;
            g_CarSwapFromIndex = 0;
            g_CarSwapToIndex = 0;
            g_CourseCardPendingGrade = 0;
            g_CourseCardSpin = 0x1000;
            g_CourseCardSpinTarget = 0x800;
            g_CourseSelectModalScript = NULL;

            sprintf(label, "== handover busy%d/offset%d/outgoing%d/course%d",
                    chosen[ch], offs[ofi], prog, courseIn[cj]);
            Record(label, NULL, 0);
            UpdateCourseSelectScreen();
            RECORD("handed", g_MenuScreen, g_MenuHandlerIndex,
                   g_MenuHandlerIndex2, g_MenuViewOffset, g_MenuViewAngle,
                   g_CarSwapToIndex, g_CourseIndex, g_UiScriptProgress);
            /* What the handover itself writes, which is the whole point of
             * reaching it: the scene it asks for and the record the race
             * reads. */
            RECORD("raceplan", g_SceneId, s_progress.course,
                   s_progress.carIndex, s_progress.classIndex,
                   s_progress.money.value);
            steps++;
        }
    }

    /*
     * The save prompt's decision, on its own. It is a pure function of the
     * press and the state it acts on, so every combination is checked here
     * directly rather than hoped for out of the sweep above: each button that
     * counts as confirm and as cancel, both directions, and the pairs, which
     * do two things and play two sounds rather than the first one only.
     */
    {
        static const struct {
            const char *what;
            u16 pressed;
            s32 subCursor;
            s32 cues[4];
            s32 cueCount;
            s32 busy;
            s32 timer;
            s32 subAfter;
        } cases[] = {
            {"nothing held", 0, 0, {0}, 0, 9, 7, 0},
            {"start confirms", PAD_START, 0, {3}, 1, -3, 0x23, 0},
            {"cross confirms", PAD_CROSS, 0, {3}, 1, -3, 0x23, 0},
            {"circle confirms", PAD_CIRCLE, 0, {3}, 1, -3, 0x23, 0},
            {"confirming on the far button", PAD_CROSS, 1, {2}, 1, -3, 0x23, 1},
            {"square cancels", PAD_SQUARE, 0, {3}, 1, -4, 7, 0},
            {"triangle cancels", PAD_TRIANGLE, 0, {3}, 1, -4, 7, 0},
            {"cancelling ignores the cursor", PAD_SQUARE, 1, {3}, 1, -4, 7, 1},
            {"left picks the far button", PAD_LEFT, 0, {1}, 1, 9, 7, 1},
            {"left again stays there", PAD_LEFT, 1, {1}, 1, 9, 7, 1},
            {"right picks the near one", PAD_RIGHT, 1, {1}, 1, 9, 7, 0},
            {"both directions land right", PAD_LEFT | PAD_RIGHT, 0,
             {1, 1}, 2, 9, 7, 0},
            /* Confirm reads the cursor before the direction moves it, so this
             * confirms the near button and then moves to the far one. */
            {"confirm with left", PAD_CROSS | PAD_LEFT, 0, {3, 1}, 2, -3,
             0x23, 1},
            {"confirm with right", PAD_CROSS | PAD_RIGHT, 1, {2, 1}, 2, -3,
             0x23, 0},
            /* Cancel is decided after confirm, so holding both cancels. */
            {"confirm and cancel together", PAD_CROSS | PAD_SQUARE, 0,
             {3, 3}, 2, -4, 0x23, 0},
            {"everything at once", PAD_CROSS | PAD_SQUARE | PAD_LEFT |
             PAD_RIGHT, 0, {3, 3, 1, 1}, 4, -4, 0x23, 0},
        };
        size_t ci;

        for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++) {
            MenuPromptOutcome got = DecideSavePrompt(
                cases[ci].pressed, 9, 7, cases[ci].subCursor);
            int k;
            if (got.cueCount != cases[ci].cueCount) {
                printf("FAIL %s: played %d sounds, expected %d\n",
                       cases[ci].what, got.cueCount, cases[ci].cueCount);
                promptFailures++;
                continue;
            }
            for (k = 0; k < got.cueCount; k++) {
                if (got.cues[k] != cases[ci].cues[k]) {
                    printf("FAIL %s: sound %d was %d, expected %d\n",
                           cases[ci].what, k, got.cues[k], cases[ci].cues[k]);
                    promptFailures++;
                }
            }
            if (got.busy != cases[ci].busy) {
                printf("FAIL %s: left the screen in %d, expected %d\n",
                       cases[ci].what, got.busy, cases[ci].busy);
                promptFailures++;
            }
            if (got.confirmTimer != cases[ci].timer) {
                printf("FAIL %s: timer %d, expected %d\n", cases[ci].what,
                       got.confirmTimer, cases[ci].timer);
                promptFailures++;
            }
            if (got.subCursor != cases[ci].subAfter) {
                printf("FAIL %s: cursor %d, expected %d\n", cases[ci].what,
                       got.subCursor, cases[ci].subAfter);
                promptFailures++;
            }
        }
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    /*
     * The class prompt, likewise. This one matters more: confirming a class
     * the player has not got resets their whole series progress, and the
     * sweep presses one button at a time, so nothing above ever holds confirm
     * together with a direction. That is the case where the curtain has to
     * start between the two sounds rather than before or after both.
     */
    {
        enum { CUE = MENU_PROMPT_CUE, CURT = MENU_PROMPT_CURTAIN };
        static const struct {
            const char *what;
            u16 pressed;
            s32 subCursor;
            s32 currentClass;
            s32 kinds[5];
            s32 values[5];
            s32 count;
            s32 busy;
            s32 timer;
            s32 applied;
            s32 subAfter;
        } cases[] = {
            {"nothing held", 0, 1, 1, {0}, {0}, 0, 9, 7, 1, 1},
            /* Choosing the class already in use must cost nothing. */
            {"confirming the class in use", PAD_CROSS, 2, 2,
             {CUE}, {2}, 1, 0, 7, 1, 2},
            {"confirming another class", PAD_CROSS, 1, 2,
             {CUE, CURT}, {2, 0}, 2, -5, 0x23, 0, 1},
            {"cancelling", PAD_SQUARE, 1, 2, {CUE}, {3}, 1, 0, 7, 1, 1},
            {"up", PAD_UP, 2, 2, {CUE}, {1}, 1, 9, 7, 1, 1},
            {"up wraps to the highest reached", PAD_UP, 0, 0,
             {CUE}, {1}, 1, 9, 7, 1, 3},
            {"down", PAD_DOWN, 1, 1, {CUE}, {1}, 1, 9, 7, 1, 2},
            {"down wraps back to the first", PAD_DOWN, 3, 3,
             {CUE}, {1}, 1, 9, 7, 1, 0},
            /* The ordering case: the curtain starts between the two sounds. */
            {"confirming another class while holding up", PAD_CROSS | PAD_UP,
             1, 2, {CUE, CURT, CUE}, {2, 0, 1}, 3, -5, 0x23, 0, 0},
            /* Confirm reads the cursor before up moves it, so this confirms
             * the class it was on and only then steps away from it. */
            {"confirming the class in use while holding up",
             PAD_CROSS | PAD_UP, 2, 2, {CUE, CUE}, {2, 1}, 2, 0, 7, 1, 1},
            {"confirm and cancel together", PAD_CROSS | PAD_SQUARE, 1, 2,
             {CUE, CURT, CUE}, {2, 0, 3}, 3, 0, 0x23, 0, 1},
        };
        size_t ci;

        for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++) {
            MenuClassPromptOutcome got = DecideClassPrompt(
                cases[ci].pressed, 9, 7, cases[ci].subCursor,
                cases[ci].currentClass, 3, 1);
            int k;
            if (got.effectCount != cases[ci].count) {
                printf("FAIL %s: did %d things, expected %d\n",
                       cases[ci].what, got.effectCount, cases[ci].count);
                promptFailures++;
                continue;
            }
            for (k = 0; k < got.effectCount; k++) {
                if (got.effects[k].kind != cases[ci].kinds[k] ||
                    got.effects[k].value != cases[ci].values[k]) {
                    printf("FAIL %s: thing %d was kind %d value %d, "
                           "expected kind %d value %d\n", cases[ci].what, k,
                           got.effects[k].kind, got.effects[k].value,
                           cases[ci].kinds[k], cases[ci].values[k]);
                    promptFailures++;
                }
            }
            if (got.busy != cases[ci].busy) {
                printf("FAIL %s: left the screen in %d, expected %d\n",
                       cases[ci].what, got.busy, cases[ci].busy);
                promptFailures++;
            }
            if (got.confirmTimer != cases[ci].timer) {
                printf("FAIL %s: timer %d, expected %d\n", cases[ci].what,
                       got.confirmTimer, cases[ci].timer);
                promptFailures++;
            }
            if (got.changeApplied != cases[ci].applied) {
                printf("FAIL %s: applied %d, expected %d\n", cases[ci].what,
                       got.changeApplied, cases[ci].applied);
                promptFailures++;
            }
            if (got.subCursor != cases[ci].subAfter) {
                printf("FAIL %s: cursor %d, expected %d\n", cases[ci].what,
                       got.subCursor, cases[ci].subAfter);
                promptFailures++;
            }
        }
    }

    /*
     * The idle screen. The asymmetry worth pinning is that confirm here acts
     * on the row the directions have just moved to, where the prompts confirm
     * the one that was showing. The sweep presses one button at a time and so
     * cannot tell the two apart.
     */
    {
        static const struct {
            const char *what;
            u16 pressed;
            u16 held;
            s32 option;
            s32 cueCount;
            s32 optionAfter;
            int prev, next, choose;
        } cases[] = {
            {"nothing", 0, 0, 1, 0, 1, 0, 0, 0},
            {"up", PAD_UP, 0, 1, 1, 0, 0, 0, 0},
            {"up wraps to the last row", PAD_UP, 0, 0, 1, 2, 0, 0, 0},
            {"down", PAD_DOWN, 0, 1, 1, 2, 0, 0, 0},
            {"down wraps to the first row", PAD_DOWN, 0, 2, 1, 0, 0, 0, 0},
            /* Both are acted on, in order, so the second undoes the first. */
            {"up and down together", PAD_UP | PAD_DOWN, 0, 1, 2, 1, 0, 0, 0},
            {"holding left asks to browse back", 0, PAD_LEFT, 1, 0, 1,
             1, 0, 0},
            {"holding right asks to browse on", 0, PAD_RIGHT, 1, 0, 1,
             0, 1, 0},
            {"holding both asks for both", 0, PAD_LEFT | PAD_RIGHT, 1, 0, 1,
             1, 1, 0},
            {"a press is not a hold", PAD_LEFT | PAD_RIGHT, 0, 1, 0, 1,
             0, 0, 0},
            {"confirm takes the row showing", PAD_CROSS, 0, 1, 0, 1, 0, 0, 1},
            /* This is the asymmetry: the row confirmed is the row moved to. */
            {"confirm with up takes the row moved to", PAD_CROSS | PAD_UP, 0,
             1, 1, 0, 0, 0, 1},
            {"confirm with down likewise", PAD_CROSS | PAD_DOWN, 0, 1, 1, 2,
             0, 0, 1},
            {"confirm after wrapping", PAD_CROSS | PAD_UP, 0, 0, 1, 2,
             0, 0, 1},
            {"confirm while browsing", PAD_CROSS, PAD_LEFT, 2, 0, 2, 1, 0, 1},
        };
        size_t ci;

        for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ci++) {
            CourseSelectInputOutcome got = DecideCourseSelectInput(
                cases[ci].pressed, cases[ci].held, cases[ci].option);
            int k;
            if (got.cueCount != cases[ci].cueCount) {
                printf("FAIL %s: played %d sounds, expected %d\n",
                       cases[ci].what, got.cueCount, cases[ci].cueCount);
                promptFailures++;
            }
            for (k = 0; k < got.cueCount && k < cases[ci].cueCount; k++) {
                if (got.cues[k] != 1) {
                    printf("FAIL %s: sound %d was %d, expected 1\n",
                           cases[ci].what, k, got.cues[k]);
                    promptFailures++;
                }
            }
            if (got.option != cases[ci].optionAfter) {
                printf("FAIL %s: row %d, expected %d\n", cases[ci].what,
                       got.option, cases[ci].optionAfter);
                promptFailures++;
            }
            if (got.wantsPrev != cases[ci].prev ||
                got.wantsNext != cases[ci].next ||
                got.choosesRow != cases[ci].choose) {
                printf("FAIL %s: prev/next/choose %d/%d/%d, expected "
                       "%d/%d/%d\n", cases[ci].what, got.wantsPrev,
                       got.wantsNext, got.choosesRow, cases[ci].prev,
                       cases[ci].next, cases[ci].choose);
                promptFailures++;
            }
        }
    }

    if (promptFailures != 0) {
        printf("%d save prompt assertion(s) failed\n", promptFailures);
        return 1;
    }
    if (s_digest != expected) {
        printf("FAIL the course select screen behaves differently: %d states "
               "making %d calls digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("the course select screen takes the same %d states it always did\n",
           steps);
    return 0;
}
