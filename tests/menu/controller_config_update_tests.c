#include <stdio.h>
#include <limits.h>

#include "game/input_internal.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/state.h"

u8 g_PadType;
u16 g_PadPressed;
s32 g_GameMode;
s32 g_AnimTimer;
ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
static ControllerSetup s_controllerSetup;
ControllerSetup *MenuControllerSetup(void) { return &s_controllerSetup; }
NegconCalibrationValue g_NegconMaxTwist;
NegconCalibrationValue g_NegconSteerPlay;
NegconCalibrationValue g_NegconSteerNeutral;
NegconCalibrationValue g_NegconNeutralI;
NegconCalibrationValue g_NegconNeutralII;
NegconCalibrationValue g_NegconNeutralL;
u8 g_NegconAxisI;
u8 g_NegconAxisII;
u8 g_NegconAxisL;
u8 g_NegconAxisSteer;

static s32 s_cues[4];
static s32 s_cueCount;
static s32 s_loadCount;
static s32 s_loadedPadMapping;
static s32 s_loadedNegconMapping;
static s32 s_configDraws;
static s32 s_neutralDraws;
static s32 s_hintVariant;
static s32 s_sceneVariant;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

void PlaySoundCue(s32 cue) {
    s_cues[s_cueCount++] = cue;
}

void LoadPadButtonMapping(s32 padMapping, s32 negconMapping) {
    s_loadCount++;
    s_loadedPadMapping = padMapping;
    s_loadedNegconMapping = negconMapping;
}

void DrawControllerConfigScreen(const ControllerSetup *setup) {
    CHECK(setup == &s_controllerSetup);
    s_configDraws++;
}

void DrawNegconNeutralScreen(void) {
    s_neutralDraws++;
}

void DrawOptionHintBar(s32 variant) {
    s_hintVariant = variant;
}

void DrawControllerSetupScene(const ControllerSetup *setup, s32 variant) {
    CHECK(setup == &s_controllerSetup);
    s_sceneVariant = variant;
}

static void ResetState(void) {
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadPressed = 0;
    g_GameMode = -1;
    g_AnimTimer = 10;
    MenuControllerSetup()->arrowPhase = 20;
    g_PadMappingIndex = 3;
    g_NegconMappingIndex = 4;
    MenuControllerSetup()->savedPadMapping = 1;
    MenuControllerSetup()->savedNegconMapping = 2;
    MenuControllerSetup()->angleX = 100;
    MenuControllerSetup()->angleY = 0;
    s_cueCount = 0;
    s_loadCount = 0;
    s_configDraws = 0;
    s_neutralDraws = 0;
    s_hintVariant = -1;
    s_sceneVariant = -1;
}

static void TestMappingSelection(void) {
    ResetState();
    g_PadPressed = PAD_LEFT;
    UpdateControllerConfigScreen();
    CHECK(g_PadMappingIndex == 2 && g_NegconMappingIndex == 4);
    CHECK(MenuControllerSetup()->angleY == 1920);
    CHECK(s_cueCount == 1 && s_cues[0] == 8);

    ResetState();
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = PAD_RIGHT;
    UpdateControllerConfigScreen();
    CHECK(g_PadMappingIndex == 3 && g_NegconMappingIndex == 5);
    CHECK(MenuControllerSetup()->angleY == -1920);
    CHECK(g_AnimTimer == 11 && MenuControllerSetup()->arrowPhase == 116);
    CHECK(s_configDraws == 1 && s_hintVariant == 1 && s_sceneVariant == 0);

    ResetState();
    g_AnimTimer = INT_MAX;
    MenuControllerSetup()->arrowPhase = INT_MAX;
    UpdateControllerConfigScreen();
    CHECK(g_AnimTimer == INT_MIN);
    CHECK(MenuControllerSetup()->arrowPhase == (s32)((u32)INT_MAX + 96u));

    ResetState();
    MenuControllerSetup()->angleY = INT_MAX;
    g_PadPressed = PAD_LEFT;
    UpdateControllerConfigScreen();
    CHECK(MenuControllerSetup()->angleY ==
          (s32)(((int64_t)(s32)((u32)INT_MAX + 2048u) * 15) / 16));
}

static void TestMappingNavigation(void) {
    ResetState();
    g_PadPressed = PAD_CANCEL;
    UpdateControllerConfigScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(g_PadMappingIndex == 1 && g_NegconMappingIndex == 2);
    CHECK(s_cueCount == 1 && s_cues[0] == 3);

    ResetState();
    g_PadPressed = PAD_CROSS;
    UpdateControllerConfigScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_loadCount == 1);
    CHECK(s_loadedPadMapping == 3 && s_loadedNegconMapping == 4);

    ResetState();
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = PAD_START;
    UpdateControllerConfigScreen();
    CHECK(g_GameMode == OPTION_MODE_NEGCON_BEGIN && s_loadCount == 1);

    ResetState();
    g_PadPressed = PAD_CANCEL | PAD_CONFIRM | PAD_LEFT;
    UpdateControllerConfigScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(g_PadMappingIndex == MenuControllerSetup()->savedPadMapping);
    CHECK(g_NegconMappingIndex == MenuControllerSetup()->savedNegconMapping);
    CHECK(s_loadCount == 0 && s_cueCount == 1 && s_cues[0] == 3);
}

static void SetCalibrationValues(void) {
    g_NegconSteerNeutral = -11;
    g_NegconNeutralI = 22;
    g_NegconNeutralII = -33;
    g_NegconNeutralL = 44;
    g_NegconSteerPlay = 2;
    g_NegconMaxTwist = 3;
}

static void TestCalibrationSnapshot(void) {
    ResetState();
    SetCalibrationValues();
    BeginNegconCalibration();
    CHECK(g_NegconSteerNeutral == 0 && g_NegconNeutralI == 0);
    CHECK(g_NegconNeutralII == 0 && g_NegconNeutralL == 0);
    CHECK(MenuControllerSetup()->angleX == 0 && MenuControllerSetup()->angleY == 0);
    CHECK(g_GameMode == OPTION_MODE_NEGCON_NEUTRAL);

    RestoreNegconCalibrationSettings();
    CHECK(g_NegconSteerNeutral == -11 && g_NegconNeutralI == 22);
    CHECK(g_NegconNeutralII == -33 && g_NegconNeutralL == 44);
    CHECK(g_NegconSteerPlay == 2 && g_NegconMaxTwist == 3);
}

static void TestNeutralCaptureAndDisconnect(void) {
    ResetState();
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = PAD_START;
    g_NegconAxisSteer = 140;
    g_NegconAxisI = 21;
    g_NegconAxisII = 31;
    g_NegconAxisL = 41;
    UpdateNegconNeutralScreen();
    CHECK(g_GameMode == OPTION_MODE_NEGCON_STEER_PLAY &&
          g_NegconSteerNeutral == 12);
    CHECK(g_NegconNeutralI == 21 && g_NegconNeutralII == 31);
    CHECK(g_NegconNeutralL == 41);
    CHECK(s_cueCount == 1 && s_cues[0] == 2);
    CHECK(s_neutralDraws == 1 &&
          s_hintVariant == MENU_OPTION_HINT_NEGCON_CALIBRATION &&
          s_sceneVariant == 0);

    ResetState();
    SetCalibrationValues();
    BeginNegconCalibration();
    g_PadType = PAD_TYPE_DIGITAL;
    UpdateNegconNeutralScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_NegconSteerNeutral == -11);
    CHECK(g_NegconNeutralI == 22 && g_NegconNeutralII == -33);
    CHECK(g_NegconNeutralL == 44);

    ResetState();
    g_PadType = PAD_TYPE_NEGCON;
    g_AnimTimer = INT_MAX;
    UpdateNegconNeutralScreen();
    CHECK(g_AnimTimer == INT_MIN);
}

int main(void) {
    TestMappingSelection();
    TestMappingNavigation();
    TestCalibrationSnapshot();
    TestNeutralCaptureAndDisconnect();
    return s_failures != 0;
}
