/*
 * The car select screen, swept.
 *
 * UpdateCarSelectScreen is three hundred lines nested ten deep, and it is the
 * hub every other menu screen is reached from: start the race, look at the
 * ranking, buy a car, take it to the engineer, back out. Nothing tested it.
 *
 * It is a state machine over fifty globals driven by the pad and by whether
 * the drawing scripts have finished, so this walks the states that decide
 * which branch runs and folds everything the call could have written, plus
 * every outward call it made, into one number.
 */

#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/save_internal.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_CarListCursor;
CarModelAsset *g_CarModelAsset;
s32 g_CarNamePlateStep;
s32 g_CarSelectCursor;
u8 g_CarSelectMenuScriptGp;
u8 g_CarSelectMenuScriptTimeAttack;
u8 *g_CarSelectPopupScript;
/* The two modal scripts are decoded command arrays rather than raw bytes;
 * they are never walked here, only identified. */
TimedDrawCommand g_CarShopUnavailableScript[2];
s32 g_CarSpecGraphStep;
s32 g_CarSwapFromIndex;
s32 g_CarSwapToIndex;
s32 g_CourseCardPendingGrade;
s32 g_CourseCardSpin;
s32 g_CourseIndex;
CourseProgressState *g_CourseProgress;
TimedDrawCommand g_EngineerShopUnavailableScript[3];
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuAltPanelStep;
s32 g_MenuAltPanelStep2;
s32 g_MenuCourseModelIndex;
s32 g_MenuHandlerIndex;
s32 g_MenuHandlerIndex2;
s32 g_MenuHintBarStep;
s32 g_MenuOutgoingScreenProgress;
s32 g_MenuOverlayPattern;
s32 g_MenuPendingCourseIndex;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s32 g_MenuViewOffset;
s32 g_MenuViewOffsetTarget;
s16 g_NextOwnedCarIndex;
u16 g_PadHeld;
u16 g_PadPressed;
s32 g_PlayerCarIndex;
s32 g_PlayerMoney;
s16 g_PrevOwnedCarIndex;
GameRaceProgress *g_RaceProgress;
s32 g_SceneId;
s32 g_ShopCarIndex;
s32 g_TimeAttackPlateStep;
u8 g_UiChromeScript;
u8 g_UiChromeScript2;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;

/* Whether a drawing script reports itself finished gates most of the screen,
 * so the sweep sets it rather than the stub deciding. */
static s32 s_scriptResult;

static void Fold(unsigned char byte) {
    s_digest = ((s_digest ^ byte) * 16777619UL) & 0xFFFFFFFFUL;
}

/*
 * The digest folds the raw values rather than their text, because the sweep
 * runs millions of times and formatting them costs more than the code under
 * test. The readable form is only produced when a file was asked for.
 */
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

/* Which script a call names matters, so each one is recorded by an index
 * rather than by a pointer that would move between runs. */
static s32 ScriptId(const void *commands) {
    if (commands == &g_CarSelectMenuScriptGp) return 1;
    if (commands == &g_CarSelectMenuScriptTimeAttack) return 2;
    if (commands == &g_UiChromeScript) return 3;
    if (commands == &g_UiChromeScript2) return 4;
    if (commands == g_CarShopUnavailableScript) return 5;
    if (commands == g_EngineerShopUnavailableScript) return 6;
    return commands == NULL ? 0 : 7;
}

s32 RunTimedDrawScript(void *commands, s32 *progress, s32 step) {
    RECORD("script", ScriptId(commands), progress == &g_UiScriptProgress2,
           step);
    return s_scriptResult;
}

void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    RECORD("nameplate", step, model, grade);
}
void DrawMenuCarView(void) { RECORD("carview", 0); }
void DrawMenuLightBurst(s32 arg) { RECORD("burst", arg); }
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight) {
    RECORD("arrows", step, wide, drawLeft, drawRight);
}
void DrawOwnedCarCounter(s32 owned, s32 step) {
    RECORD("counter", owned, step);
}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    RECORD("sprites", progress, count, slot);
}
s32 CountOwnedCars(void) { RECORD("countowned", 0); return 6; }
void UpdateOwnedCarNeighbours(void) { RECORD("neighbours", 0); }
void RefreshCarUnlockState(void) { RECORD("unlockstate", 0); }
void RequestCarModel(s32 carIndex) { RECORD("requestcar", carIndex); }
void StartSequenceFadeOut(void) { RECORD("fadeout", 0); }
s32 RequestRoundAssets(void) { RECORD("roundassets", 0); return 0; }
void PlaySoundCue(s32 cue) { RECORD("cue", cue); }
s32 GetCarUnlockLevel(s32 model) { RECORD("unlocklevel", model); return 3; }
void DrawCarShopPricePanel(s32 step, s32 money, s32 price) {
    RECORD("shopprice", step, money, price);
}
void DrawMenuAltPanel(s32 stepA, s32 stepB) { RECORD("altpanel", stepA, stepB); }
void ClearTeamNameTexture(void) { RECORD("clearteamname", 0); }
void RestoreTeamLogoClut(void) { RECORD("restoreclut", 0); }
void DrawEngineerShopPricePanel(s32 step, s32 money, s32 price) {
    RECORD("engineerprice", step, money, price);
}
void DrawTimeAttackPlate(s32 stepArg) { RECORD("timeattackplate", stepArg); }

static GameRaceProgress s_progress;
static CourseProgressState s_course;
static CarModelAsset s_model;

int main(int argc, char **argv) {
    /*
     * What the screen did before it was taken apart. Run the test with a file
     * name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 1048880757UL;
    static const s32 busyStates[] = {0, -1, 1, 2, 3, 4, 5};
    static const u16 buttons[] = {0, PAD_UP, PAD_DOWN, PAD_CONFIRM, PAD_CANCEL};
    /* Both directions at once is unreachable on a d-pad but not in the
     * code, and it is the one case where the two swaps interact. */
    static const u16 held[] = {0, PAD_LEFT, PAD_RIGHT,
                               PAD_LEFT | PAD_RIGHT};
    static const s32 cursors[] = {0, 1, 2, 3, 4};
    static const s32 owned[] = {-1, 5};
    static const s32 offsets[] = {0x3D08F, 0x3D090};
    static const s32 settledOffsets[] = {0, 0x493DF, 0x493E0, -0x493DF};
    int bi, sr, p2, gp, ci, pb, hb, settled, swap, oi, shop, upgrade, prog, off;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_RaceProgress = &s_progress;
    g_CourseProgress = &s_course;
    g_CarModelAsset = &s_model;

    for (bi = 0; bi < 7; bi++)
    for (sr = 0; sr < 2; sr++)
    for (p2 = 0; p2 < 2; p2++)
    for (gp = 0; gp < 2; gp++)
    for (ci = 0; ci < 5; ci++)
    for (pb = 0; pb < 5; pb++)
    for (hb = 0; hb < 4; hb++)
    for (settled = 0; settled < 4; settled++)
    for (swap = 0; swap < 2; swap++)
    for (oi = 0; oi < 2; oi++)
    for (shop = 0; shop < 2; shop++)
    for (upgrade = 0; upgrade < 2; upgrade++)
    for (prog = 0; prog < 2; prog++)
    for (off = 0; off < 2; off++) {
        char label[192];

        memset(&s_progress, 0, sizeof(s_progress));
        memset(&s_course, 0, sizeof(s_course));
        memset(&s_model, 0, sizeof(s_model));
        s_course.bestPlace[0] = 1;
        s_course.bestPlace[1] = 2;
        s_course.bestPlace[2] = 3;
        s_course.bestPlace[3] = 4;
        s_model.upgradesAvailable = (u8)upgrade;
        s_progress.maxClassReached = upgrade ? 4 : 1;

        GameMenuBusy = busyStates[bi];
        s_scriptResult = sr;
        g_UiScriptProgress2 = p2;
        g_UiScriptProgress = prog;
        g_GrandPrixMode = (s16)gp;
        g_CarSelectCursor = cursors[ci];
        g_PadPressed = buttons[pb];
        g_PadHeld = held[hb];
        /* The screen only lets a car be swapped once the view has come to rest
         * within 0x493DF of its target, so the sweep sits on both sides of
         * that distance and on both sides of the target. */
        g_MenuViewAngleTarget = 0x7A120;
        g_MenuViewAngle = 0x7A120 + settledOffsets[settled];
        g_CarSwapToIndex = swap ? -1 : 3;
        g_PrevOwnedCarIndex = (s16)owned[oi];
        g_NextOwnedCarIndex = (s16)owned[1 - oi];
        g_ShopCarIndex = shop ? -1 : 7;
        g_MenuViewOffset = offsets[off];
        g_MenuOutgoingScreenProgress = off;

        g_MenuAltLayoutSetting = 1;
        g_CarNamePlateStep = 4;
        g_CarSpecGraphStep = 1;
        g_MenuPlateCarIndex = 2;
        g_PlayerCarIndex = 9;
        g_CarListCursor = 0;
        g_CarSwapFromIndex = 0;
        g_CourseIndex = 6;
        g_GrandPrixClass = gp ? 5 : 2;
        g_GrandPrixSeries = 3;
        g_MenuAltPanelStep = 0;
        g_MenuAltPanelStep2 = 0;
        g_MenuCourseModelIndex = 0;
        g_MenuHandlerIndex = 0;
        g_MenuHandlerIndex2 = 0;
        g_MenuHintBarStep = 0;
        g_MenuOverlayPattern = 0;
        g_MenuPendingCourseIndex = 0;
        g_MenuScreen = 0;
        g_MenuViewOffsetTarget = 0;
        g_PlayerMoney = 12345;
        g_SceneId = 0;
        g_TimeAttackPlateStep = 0;
        g_CourseCardPendingGrade = 0;
        g_CourseCardSpin = 0;
        g_CarSelectPopupScript = NULL;

        sprintf(label,
                "== busy%d/script%d/p2_%d/gp%d/cur%d/pad%04x/held%04x/"
                "settled%d/swap%d/own%d/shop%d/up%d/prog%d/off%d",
                busyStates[bi], sr, p2, gp, cursors[ci], buttons[pb], held[hb],
                settledOffsets[settled], swap, oi, shop, upgrade, prog, off);
        Record(label, NULL, 0);

        UpdateCarSelectScreen();

        {
            s32 after[22];
            after[0] = GameMenuBusy;
            after[1] = g_CarSelectCursor;
            after[2] = g_PlayerCarIndex;
            after[3] = g_CarListCursor;
            after[4] = g_CarSwapFromIndex;
            after[5] = g_CarSwapToIndex;
            after[6] = g_MenuViewAngle;
            after[7] = g_MenuViewAngleTarget;
            after[8] = g_MenuViewOffsetTarget;
            after[9] = g_MenuScreen;
            after[10] = g_MenuHandlerIndex;
            after[11] = g_MenuHandlerIndex2;
            after[12] = g_MenuOverlayPattern;
            after[13] = g_CarNamePlateStep;
            after[14] = g_CarSpecGraphStep;
            after[15] = g_MenuHintBarStep;
            after[16] = g_SceneId;
            after[17] = g_CourseIndex;
            after[18] = g_GrandPrixSeries;
            after[19] = g_TimeAttackPlateStep;
            after[20] = ScriptId(g_CarSelectPopupScript);
            after[21] = g_MenuAltLayout;
            Record("state", after, 22);
            RECORD("saved", s_progress.course, s_progress.carIndex,
                   s_progress.classIndex, s_progress.money.value);
        }
        steps++;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL the car select screen behaves differently: %d states "
               "making %d calls digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("the car select screen takes the same %d states it always did\n",
           steps);
    return 0;
}
