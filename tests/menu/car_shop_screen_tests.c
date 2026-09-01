/*
 * The car shop, swept.
 *
 * UpdateCarShopScreen is the only place in the game where money changes hands.
 * It is two hundred and seventy lines nested nine deep, holding the showroom
 * turntable, the buy prompt with its own yes/no cursor, the "you cannot afford
 * this" refusal, a confirmation countdown, and the write that marks a car as
 * owned. Nothing tested any of it.
 *
 * So this walks the states that decide which branch runs and folds everything
 * the call could have written, plus every outward call it made, into one
 * number.
 */

#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_CarListCursor;
CarModelAsset *g_CarModelAsset;
s32 g_CarNamePlateStep;
s32 g_CarPriceTable[16];
/* The four buy prompts and the refusals are decoded command arrays; they are
 * never walked here, only identified. */
TimedDrawCommand g_CarShopBuyPromptScript1[7];
TimedDrawCommand g_CarShopBuyPromptScript2[7];
TimedDrawCommand g_CarShopBuyPromptScript3[7];
TimedDrawCommand g_CarShopBuyPromptScript4[7];
TimedDrawCommand g_CarShopNoFundsScript[5];
u8 *g_CarShopModalScript;
s32 g_CarShopOption;
TimedDrawCommand g_CarShopScreenScript[9];
s32 g_CarSwapFromIndex;
s32 g_CarSwapToIndex;
CarEntry *g_CarTable;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuAltPanelStep;
s32 g_MenuAltPanelStep2;
u8 g_MenuBlankCaption;
s32 g_MenuConfirmTimer;
s32 g_MenuHandlerIndex;
s32 g_MenuHandlerIndex2;
s32 g_MenuOverlayPattern;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
u8 g_MenuSubCursor;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s16 g_NextOwnedCarIndex;
volatile u16 g_PadHeld;
u16 g_PadPressed;
s32 g_PlayerCarIndex;
s32 g_PlayerMoney;
s16 g_PrevOwnedCarIndex;
u8 g_TeamNameChars[16];
u8 g_TeamNameLength;
u8 g_TimeAttackCarEnabled[128];
u8 g_UiChromeScript;
u8 g_UiChromeScript2;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;
GameRenderState g_RenderState;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;
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

static s32 ScriptId(const void *commands) {
    if (commands == g_CarShopScreenScript) return 1;
    if (commands == &g_UiChromeScript) return 2;
    if (commands == &g_UiChromeScript2) return 3;
    if (commands == g_CarShopBuyPromptScript1) return 4;
    if (commands == g_CarShopBuyPromptScript2) return 5;
    if (commands == g_CarShopBuyPromptScript3) return 6;
    if (commands == g_CarShopBuyPromptScript4) return 7;
    if (commands == g_CarShopNoFundsScript) return 8;
    return commands == NULL ? 0 : 9;
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
void DrawMenuAltPanel(s32 stepA, s32 stepB) { RECORD("altpanel", stepA, stepB); }
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight) {
    RECORD("arrows", step, wide, drawLeft, drawRight);
}
void DrawCarShopPricePanel(s32 step, s32 money, s32 price) {
    RECORD("price", step, money, price);
}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    RECORD("sprites", progress, count, slot);
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
s32 GetOwnedCarAssetIndex(s32 model) {
    RECORD("assetindex", model);
    return model & 7;
}
void UpdateCarListCursor(void) { RECORD("listcursor", 0); }
void RequestCarModel(s32 carIndex) { RECORD("requestcar", carIndex); }
void PlaySoundCue(s32 cue) { RECORD("cue", cue); }
void UploadTeamNameTexture(u8 *str, s32 len) {
    RECORD("teamname", str == g_TeamNameChars, len);
}
void UploadTeamLogoClut(void) { RECORD("teamclut", 0); }

static CarModelAsset s_model;
static CarEntry s_cars[16];

int main(int argc, char **argv) {
    /*
     * What the shop did before it was taken apart. Run the test with a file
     * name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 3036908947UL;
    static const s32 busyStates[] = {0, -1, -2, -3, 1, 2};
    static const u16 buttons[] = {0, PAD_UP, PAD_DOWN, PAD_CONFIRM, PAD_CANCEL,
                                  0x8000, 0x0080, 0x0010};
    static const u16 held[] = {0, PAD_LEFT, PAD_RIGHT};
    /* Which buy prompt a car gets depends on the band it falls in; car 13
     * falls through the switch entirely and gets no prompt at all. */
    static const s32 cars[] = {0, 3, 4, 7, 13};
    static const s32 owned[] = {-1, 5};
    /* The turntable's rest window is 0x493DF wide, so the sweep sits on both
     * sides of it and on both sides of the target. */
    static const s32 settledOffsets[] = {0, 0x493DF, 0x493E0, -0x493DF};
    s32 ot[64];
    int bi, sr, p2, opt, pb, hb, settled, swap, oi, ci, sub, rich, timer,
        transmission, prog, same;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_CarModelAsset = &s_model;
    g_CarTable = s_cars;

    for (bi = 0; bi < 6; bi++)
    for (sr = 0; sr < 2; sr++)
    for (p2 = 0; p2 < 2; p2++)
    for (opt = 0; opt < 2; opt++)
    for (pb = 0; pb < 8; pb++)
    for (hb = 0; hb < 3; hb++)
    for (settled = 0; settled < 4; settled++)
    for (swap = 0; swap < 2; swap++)
    for (oi = 0; oi < 2; oi++)
    for (ci = 0; ci < 5; ci++)
    for (sub = 0; sub < 2; sub++)
    for (rich = 0; rich < 2; rich++)
    for (timer = 0; timer < 2; timer++)
    for (transmission = 0; transmission < 2; transmission++)
    for (prog = 0; prog < 2; prog++)
    for (same = 0; same < 2; same++) {
        char label[224];
        int i;

        memset(&s_model, 0, sizeof(s_model));
        memset(s_cars, 0, sizeof(s_cars));
        memset(g_TimeAttackCarEnabled, 0, sizeof(g_TimeAttackCarEnabled));
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE_AS(void) = ot;
        for (i = 0; i < 16; i++) {
            g_CarPriceTable[i] = 1000 * (i + 1);
            s_cars[i].enabled = (u8)(i & 1);
        }
        s_model.transmissionAvailable = (u8)transmission;
        g_TeamNameLength = 4;

        GameMenuBusy = busyStates[bi];
        s_scriptResult = sr;
        g_UiScriptProgress2 = p2;
        g_UiScriptProgress = prog;
        g_CarShopOption = opt;
        g_PadPressed = buttons[pb];
        g_PadHeld = held[hb];
        g_MenuViewAngleTarget = 0x7A120;
        g_MenuViewAngle = 0x7A120 + settledOffsets[settled];
        g_CarSwapToIndex = swap ? -1 : 3;
        g_PrevOwnedCarIndex = (s16)owned[oi];
        g_NextOwnedCarIndex = (s16)owned[1 - oi];
        g_CarListCursor = cars[ci];
        g_PlayerCarIndex = same ? cars[ci] : 9;
        g_MenuSubCursor = (u8)sub;
        /* Either side of the price of the car the cursor is on. */
        g_PlayerMoney = rich ? 1000000 : 0;
        g_MenuConfirmTimer = timer;

        g_MenuAltLayoutSetting = 1;
        g_CarNamePlateStep = 4;
        g_MenuPlateCarIndex = 2;
        g_CarSwapFromIndex = 0;
        g_MenuAltPanelStep = 0;
        g_MenuAltPanelStep2 = 0;
        g_MenuHandlerIndex = 0;
        g_MenuHandlerIndex2 = 0;
        g_MenuOverlayPattern = 0;
        g_MenuScreen = 0;
        g_CarShopModalScript = NULL;

        sprintf(label,
                "== busy%d/script%d/p2_%d/opt%d/pad%04x/held%04x/settled%d/"
                "swap%d/own%d/car%d/sub%d/rich%d/timer%d/trans%d/prog%d/"
                "same%d",
                busyStates[bi], sr, p2, opt, buttons[pb], held[hb],
                settledOffsets[settled], swap, oi, cars[ci], sub, rich, timer,
                transmission, prog, same);
        Record(label, NULL, 0);

        UpdateCarShopScreen();

        {
            s32 after[18];
            after[0] = GameMenuBusy;
            after[1] = g_CarShopOption;
            after[2] = g_CarListCursor;
            after[3] = g_PlayerCarIndex;
            after[4] = g_PlayerMoney;
            after[5] = g_CarSwapFromIndex;
            after[6] = g_CarSwapToIndex;
            after[7] = g_MenuViewAngle;
            after[8] = g_MenuViewAngleTarget;
            after[9] = g_MenuSubCursor;
            after[10] = g_MenuConfirmTimer;
            after[11] = g_MenuAltPanelStep;
            after[12] = g_MenuAltPanelStep2;
            after[13] = g_MenuOverlayPattern;
            after[14] = g_MenuScreen;
            after[15] = g_MenuHandlerIndex;
            after[16] = g_MenuHandlerIndex2;
            after[17] = ScriptId(g_CarShopModalScript);
            Record("state", after, 18);
            RECORD("owned", s_cars[cars[ci]].enabled,
                   g_TimeAttackCarEnabled[cars[ci] * 8], g_UiScriptProgress,
                   g_MenuPlateCarIndex, g_MenuAltLayout);
        }
        steps++;
    }

    /*
     * Two things the sweep above cannot reach without multiplying out: every
     * car index, because each one picks its buy prompt by name in a switch,
     * and the exact price, because affording a car is a >= test. Both get a
     * narrow pass of their own instead.
     */
    for (ci = 0; ci < 16; ci++) {
        char label[64];
        int i;

        memset(&s_model, 0, sizeof(s_model));
        memset(s_cars, 0, sizeof(s_cars));
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE_AS(void) = ot;
        for (i = 0; i < 16; i++) {
            g_CarPriceTable[i] = 1000 * (i + 1);
        }
        GameMenuBusy = 0;
        s_scriptResult = 1;
        g_UiScriptProgress2 = 0;
        g_CarShopOption = 0;
        g_PadPressed = PAD_CONFIRM;
        g_PadHeld = 0;
        g_MenuViewAngle = 0x7A120;
        g_MenuViewAngleTarget = 0x7A120;
        g_CarSwapToIndex = -1;
        g_CarListCursor = ci;
        g_PlayerCarIndex = ci;
        g_CarShopModalScript = NULL;

        sprintf(label, "== prompt for car %d", ci);
        Record(label, NULL, 0);
        UpdateCarShopScreen();
        RECORD("prompt", ScriptId(g_CarShopModalScript), GameMenuBusy,
               g_MenuSubCursor);
        steps++;
    }

    /* Both directions held at once is unreachable on a d-pad but not in the
     * code, and it is the one case where the two swaps interact. */
    for (oi = 0; oi < 2; oi++) {
        int i;

        memset(&s_model, 0, sizeof(s_model));
        memset(s_cars, 0, sizeof(s_cars));
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE_AS(void) = ot;
        for (i = 0; i < 16; i++) {
            g_CarPriceTable[i] = 1000 * (i + 1);
        }
        GameMenuBusy = 0;
        s_scriptResult = 1;
        g_UiScriptProgress2 = 0;
        g_CarShopOption = 0;
        g_PadPressed = 0;
        g_PadHeld = PAD_LEFT | PAD_RIGHT;
        g_MenuViewAngle = 0x7A120;
        g_MenuViewAngleTarget = 0x7A120;
        g_CarSwapToIndex = -1;
        g_CarListCursor = 4;
        g_PlayerCarIndex = 4;
        g_PrevOwnedCarIndex = (s16)owned[oi];
        g_NextOwnedCarIndex = (s16)owned[1 - oi];
        g_CarShopModalScript = NULL;

        Record("== both directions held", NULL, 0);
        UpdateCarShopScreen();
        RECORD("bothheld", g_CarListCursor, g_CarSwapFromIndex,
               g_CarSwapToIndex, g_MenuViewAngle, g_MenuViewAngleTarget);
        steps++;
    }

    for (rich = 0; rich < 3; rich++) {
        char label[64];
        int i;

        memset(&s_model, 0, sizeof(s_model));
        memset(s_cars, 0, sizeof(s_cars));
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE_AS(void) = ot;
        for (i = 0; i < 16; i++) {
            g_CarPriceTable[i] = 1000 * (i + 1);
        }
        GameMenuBusy = -1;
        s_scriptResult = 1;
        g_UiScriptProgress2 = 0;
        g_PadPressed = PAD_CONFIRM;
        g_PadHeld = 0;
        g_MenuSubCursor = 1;
        g_CarListCursor = 4;
        g_PlayerCarIndex = 4;
        g_CarShopModalScript = NULL;
        /* Car 4 costs g_CarPriceTable[4 & 7], one short of it and one over. */
        g_PlayerMoney = g_CarPriceTable[4] + (rich - 1);

        sprintf(label, "== money %d", g_PlayerMoney);
        Record(label, NULL, 0);
        UpdateCarShopScreen();
        RECORD("afford", GameMenuBusy, g_MenuConfirmTimer,
               ScriptId(g_CarShopModalScript));
        steps++;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL the car shop behaves differently: %d states making %d "
               "calls digest to %lu, expected %lu\n", steps, s_calls, s_digest,
               expected);
        return 1;
    }
    printf("the car shop takes the same %d states it always did\n", steps);
    return 0;
}
