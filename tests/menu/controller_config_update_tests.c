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
s32 g_SetupArrowPulse;
ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
u16 g_PadMappingIndexSaved;
u16 g_NegconMappingIndexSaved;
s32 g_ControllerSceneAngleX;
s32 g_ControllerSceneAngleY;
NegconCalibrationValue g_NegconMaxTwist;
NegconCalibrationValue g_NegconSteerPlay;
NegconCalibrationValue g_NegconSteerNeutral;
NegconCalibrationValue g_NegconNeutralI;
NegconCalibrationValue g_NegconNeutralII;
NegconCalibrationValue g_NegconNeutralL;
u16 g_NegconMaxTwistSaved;
u16 g_NegconSteerPlaySaved;
u16 g_NegconSteerNeutralSaved;
u16 g_NegconNeutralISaved;
u16 g_NegconNeutralIISaved;
u16 g_NegconNeutralLSaved;
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

void DrawControllerConfigScreen(void) {
    s_configDraws++;
}

void DrawNegconNeutralScreen(void) {
    s_neutralDraws++;
}

void DrawOptionHintBar(s32 variant) {
    s_hintVariant = variant;
}

void DrawControllerSetupScene(s32 variant) {
    s_sceneVariant = variant;
}

static void ResetState(void) {
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadPressed = 0;
    g_GameMode = -1;
    g_AnimTimer = 10;
    g_SetupArrowPulse = 20;
    g_PadMappingIndex = 3;
    g_NegconMappingIndex = 4;
    g_PadMappingIndexSaved = 1;
    g_NegconMappingIndexSaved = 2;
    g_ControllerSceneAngleX = 100;
    g_ControllerSceneAngleY = 0;
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
    CHECK(g_ControllerSceneAngleY == 1920);
    CHECK(s_cueCount == 1 && s_cues[0] == 8);

    ResetState();
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = PAD_RIGHT;
    UpdateControllerConfigScreen();
    CHECK(g_PadMappingIndex == 3 && g_NegconMappingIndex == 5);
    CHECK(g_ControllerSceneAngleY == -1920);
    CHECK(g_AnimTimer == 11 && g_SetupArrowPulse == 116);
    CHECK(s_configDraws == 1 && s_hintVariant == 1 && s_sceneVariant == 0);

    ResetState();
    g_AnimTimer = INT_MAX;
    g_SetupArrowPulse = INT_MAX;
    UpdateControllerConfigScreen();
    CHECK(g_AnimTimer == INT_MIN);
    CHECK(g_SetupArrowPulse == (s32)((u32)INT_MAX + 96u));

    ResetState();
    g_ControllerSceneAngleY = INT_MAX;
    g_PadPressed = PAD_LEFT;
    UpdateControllerConfigScreen();
    CHECK(g_ControllerSceneAngleY ==
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
    CHECK(g_PadMappingIndex == g_PadMappingIndexSaved);
    CHECK(g_NegconMappingIndex == g_NegconMappingIndexSaved);
    CHECK(s_loadCount == 0 && s_cueCount == 1 && s_cues[0] == 3);
}

static void SetCalibrationValues(void) {
    g_NegconSteerNeutral = 11;
    g_NegconNeutralI = 22;
    g_NegconNeutralII = 33;
    g_NegconNeutralL = 44;
    g_NegconSteerPlay = 2;
    g_NegconMaxTwist = 3;
}

static void TestCalibrationSnapshot(void) {
    ResetState();
    SetCalibrationValues();
    BeginNegconCalibration();
    CHECK(g_NegconSteerNeutralSaved == 11 && g_NegconNeutralISaved == 22);
    CHECK(g_NegconNeutralIISaved == 33 && g_NegconNeutralLSaved == 44);
    CHECK(g_NegconSteerPlaySaved == 2 && g_NegconMaxTwistSaved == 3);
    CHECK(g_NegconSteerNeutral == 0 && g_NegconNeutralI == 0);
    CHECK(g_NegconNeutralII == 0 && g_NegconNeutralL == 0);
    CHECK(g_ControllerSceneAngleX == 0 && g_ControllerSceneAngleY == 0);
    CHECK(g_GameMode == OPTION_MODE_NEGCON_NEUTRAL);

    RestoreNegconCalibrationSettings();
    CHECK(g_NegconSteerNeutral == 11 && g_NegconNeutralI == 22);
    CHECK(g_NegconNeutralII == 33 && g_NegconNeutralL == 44);
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
    CHECK(s_neutralDraws == 1 && s_hintVariant == 4 && s_sceneVariant == 0);

    ResetState();
    SetCalibrationValues();
    BeginNegconCalibration();
    g_PadType = PAD_TYPE_DIGITAL;
    UpdateNegconNeutralScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_NegconSteerNeutral == 11);
    CHECK(g_NegconNeutralI == 22 && g_NegconNeutralII == 33);
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
