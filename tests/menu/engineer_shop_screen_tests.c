/*
 * The engineer's shop, swept.
 *
 * The other place money leaves the player's pocket: a tune-up raises the car's
 * model variant by one, permanently, and the write that does it sits five
 * levels down a hundred and fifty line function nothing tested. The shop has
 * the same shape as the car shop, with a prompt, a refusal, and a countdown,
 * so this locks it the same way: walk the states that decide which branch
 * runs, and fold everything written and every call made into one number.
 */

#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_CarNamePlateStep;
s32 g_CarSwapFromIndex;
s32 g_CarSwapToIndex;
CarEntry *g_CarTable;
s32 g_CarTuneUpPriceTable[16];
const TimedDrawCommand *g_EngineerShopModalScript;
/* The prompts are decoded command arrays; never walked here, only named. */
TimedDrawCommand g_EngineerShopNoFundsScript[2];
TimedDrawCommand g_EngineerShopScreenScript[68];
TimedDrawCommand g_EngineerShopTuneUpPromptScript[5];
s32 g_EngineerShopOption;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
u8 g_MenuBlankCaption;
s32 g_MenuConfirmTimer;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
u8 g_MenuSubCursor;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
u16 g_PadPressed;
s32 g_PlayerCarIndex;
s32 g_PlayerMoney;
CarEntry g_TimeAttackCars[16];
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
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
    if (commands == g_EngineerShopScreenScript) return 1;
    if (commands == g_UiChromeScript) return 2;
    if (commands == g_UiChromeScript2) return 3;
    if (commands == g_EngineerShopTuneUpPromptScript) return 4;
    if (commands == g_EngineerShopNoFundsScript) return 5;
    return commands == NULL ? 0 : 6;
}

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress, s32 step) {
    RECORD("script", ScriptId(commands), progress == &g_UiScriptProgress2,
           step);
    return s_scriptResult;
}

void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    RECORD("nameplate", step, model, grade);
}
void DrawMenuCarView(void) { RECORD("carview", 0); }
void DrawEngineerShopPricePanel(s32 step, s32 money, s32 price) {
    RECORD("price", step, money, price);
}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    RECORD("sprites", progress, count, slot);
}
void DrawMenuCursorBox(s32 x0, s32 y0, s32 x1, s32 y1, s32 flash) {
    RECORD("cursorbox", x0, y0, x1, y1, flash);
}
void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0, u8 r,
                u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    RECORD("sprite", x0, y0, x1, y1, u0, v0, r, g, b, clutX, shadeTex,
           semiTrans, (s32)flags);
}
void GameDrawMenuButton(s32 x0, s32 y0, s32 x1, s32 y1, u8 r, u8 g, u8 b) {
    RECORD("button", x0, y0, x1, y1, r, g, b);
}
s32 GetOwnedCarAssetIndex(s32 model) {
    RECORD("assetindex", model);
    return model & 7;
}
void RequestUpgradedCarModel(s32 carIndex) {
    RECORD("upgradedmodel", carIndex);
}
void PlaySoundCue(s32 cue) { RECORD("cue", cue); }

static CarEntry s_cars[16];

int main(int argc, char **argv) {
    /*
     * What the shop did before it was taken apart. Run the test with a file
     * name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 1502267853UL;
    static const s32 busyStates[] = {0, -1, -2, -3, 1, 2};
    static const u16 buttons[] = {0, PAD_UP, PAD_DOWN, PAD_CONFIRM, PAD_CANCEL,
                                  PAD_LEFT, PAD_RIGHT};
    static const s32 cars[] = {0, 5, 9};
    GameOrderingTableEntry ot[64];
    int bi, sr, p2, opt, pb, sub, rich, timer, prog, ci, variant;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_CarTable = s_cars;

    for (bi = 0; bi < 6; bi++)
    for (sr = 0; sr < 2; sr++)
    for (p2 = 0; p2 < 2; p2++)
    for (opt = 0; opt < 3; opt++)
    for (pb = 0; pb < 7; pb++)
    for (sub = 0; sub < 2; sub++)
    for (rich = 0; rich < 3; rich++)
    for (timer = 0; timer < 2; timer++)
    for (prog = 0; prog < 2; prog++)
    for (ci = 0; ci < 3; ci++)
    /* Whether the tune-up beats the best the player has ever had on this car
     * decides if the time attack copy is raised too. */
    for (variant = 0; variant < 3; variant++) {
        char label[192];
        int i;

        memset(s_cars, 0, sizeof(s_cars));
        memset(g_TimeAttackCars, 0, sizeof(g_TimeAttackCars));
        memset(ot, 0, sizeof(ot));
        RENDER_OT_BASE = ot;
        for (i = 0; i < 16; i++) {
            g_CarTuneUpPriceTable[i] = 1000 * (i + 1);
        }
        s_cars[cars[ci]].modelVariant = 2;
        g_TimeAttackCars[cars[ci]].modelVariant = (u8)(variant + 1);

        GameMenuBusy = busyStates[bi];
        s_scriptResult = sr;
        g_UiScriptProgress2 = p2;
        g_UiScriptProgress = prog;
        g_EngineerShopOption = opt;
        g_PadPressed = buttons[pb];
        g_MenuSubCursor = (u8)sub;
        g_PlayerCarIndex = cars[ci];
        /* One short of the asking price, exactly it, and comfortably over. */
        g_PlayerMoney = g_CarTuneUpPriceTable[cars[ci] & 7] + (rich - 1);
        g_MenuConfirmTimer = timer;

        g_MenuAltLayoutSetting = 1;
        g_CarNamePlateStep = 4;
        g_MenuPlateCarIndex = 0;
        g_CarSwapFromIndex = 0;
        g_CarSwapToIndex = 0;
        g_MenuHandlerIndex = 0;
        g_MenuOutgoingHandlerIndex = 0;
        g_MenuOverlayPattern = 0;
        g_MenuScreen = 0;
        g_MenuViewAngle = 0;
        g_MenuViewAngleTarget = 0;
        g_EngineerShopModalScript = NULL;

        sprintf(label,
                "== busy%d/script%d/p2_%d/opt%d/pad%04x/sub%d/rich%d/timer%d/"
                "prog%d/car%d/variant%d",
                busyStates[bi], sr, p2, opt, buttons[pb], sub, rich - 1, timer,
                prog, cars[ci], variant + 1);
        Record(label, NULL, 0);

        UpdateEngineerShopScreen();

        {
            s32 after[14];
            after[0] = GameMenuBusy;
            after[1] = g_EngineerShopOption;
            after[2] = g_PlayerMoney;
            after[3] = g_MenuSubCursor;
            after[4] = g_MenuConfirmTimer;
            after[5] = g_MenuOverlayPattern;
            after[6] = g_MenuScreen;
            after[7] = g_MenuHandlerIndex;
            after[8] = g_MenuOutgoingHandlerIndex;
            after[9] = g_MenuViewAngle;
            after[10] = g_MenuViewAngleTarget;
            after[11] = g_CarSwapFromIndex;
            after[12] = g_CarSwapToIndex;
            after[13] = ScriptId(g_EngineerShopModalScript);
            Record("state", after, 14);
            RECORD("car", s_cars[cars[ci]].modelVariant,
                   g_TimeAttackCars[cars[ci]].modelVariant, g_UiScriptProgress,
                   g_MenuPlateCarIndex, g_MenuAltLayout);
        }
        steps++;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL the engineer's shop behaves differently: %d states making "
               "%d calls digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("the engineer's shop takes the same %d states it always did\n",
           steps);
    return 0;
}
