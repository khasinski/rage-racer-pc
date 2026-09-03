#include <stdio.h>
#include <limits.h>

#include "game/input_internal.h"
#include "game/state.h"

u8 g_PadType;
u16 g_PadPressed;
s32 g_GameMode;
s32 g_AnimTimer;
s32 g_SetupArrowPulse;
s32 g_ControllerSceneAngleX;
NegconCalibrationValue g_NegconSteerPlay;
NegconCalibrationValue g_NegconMaxTwist;

static s32 s_soundCues[4];
static s32 s_soundCueCount;
static s32 s_restoreCount;
static s32 s_steerDrawCount;
static s32 s_twistDrawCount;
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
    s_soundCues[s_soundCueCount++] = cue;
}

void RestoreNegconCalibrationSettings(void) {
    s_restoreCount++;
}

void DrawNegconSteerPlayScreen(void) {
    s_steerDrawCount++;
}

void DrawNegconMaxTwistScreen(void) {
    s_twistDrawCount++;
}

void DrawOptionHintBar(s32 variant) {
    s_hintVariant = variant;
}

void DrawControllerSetupScene(s32 variant) {
    s_sceneVariant = variant;
}

static void ResetState(void) {
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = 0;
    g_GameMode = -1;
    g_AnimTimer = 10;
    g_SetupArrowPulse = 20;
    g_ControllerSceneAngleX = 0;
    g_NegconSteerPlay = 2;
    g_NegconMaxTwist = 2;
    s_soundCueCount = 0;
    s_restoreCount = 0;
    s_steerDrawCount = 0;
    s_twistDrawCount = 0;
    s_hintVariant = -1;
    s_sceneVariant = -1;
}

static void CheckSharedFrame(s32 expectedSteerDraws, s32 expectedTwistDraws) {
    CHECK(g_ControllerSceneAngleX == -896);
    CHECK(s_steerDrawCount == expectedSteerDraws);
    CHECK(s_twistDrawCount == expectedTwistDraws);
    CHECK(s_hintVariant == 4);
    CHECK(s_sceneVariant == 1);
}

static void TestSteerNavigation(void) {
    ResetState();
    g_PadPressed = PAD_CONFIRM;
    UpdateNegconSteerPlayScreen();
    CHECK(g_GameMode == OPTION_MODE_NEGCON_MAX_TWIST);
    CHECK(g_AnimTimer == 11);
    CHECK(g_SetupArrowPulse == 116);
    CHECK(s_soundCueCount == 1 && s_soundCues[0] == 2);
    CHECK(s_restoreCount == 0);
    CheckSharedFrame(1, 0);

    ResetState();
    g_PadPressed = PAD_CANCEL;
    UpdateNegconSteerPlayScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(s_soundCueCount == 1 && s_soundCues[0] == 3);
    CHECK(s_restoreCount == 1);

    ResetState();
    g_AnimTimer = INT_MAX;
    g_SetupArrowPulse = INT_MAX;
    UpdateNegconSteerPlayScreen();
    CHECK(g_AnimTimer == INT_MIN);
    CHECK(g_SetupArrowPulse == (s32)((u32)INT_MAX + 96u));
}

static void TestTwistNavigation(void) {
    ResetState();
    g_PadPressed = PAD_CONFIRM;
    UpdateNegconMaxTwistScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(g_AnimTimer == 11);
    CHECK(g_SetupArrowPulse == 20);
    CHECK(s_soundCueCount == 1 && s_soundCues[0] == 2);
    CHECK(s_restoreCount == 0);
    CheckSharedFrame(0, 1);

    ResetState();
    g_PadPressed = PAD_CANCEL;
    UpdateNegconMaxTwistScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(s_soundCueCount == 1 && s_soundCues[0] == 3);
    CHECK(s_restoreCount == 1);
}

static void TestSettingRangeAndInputOrder(void) {
    ResetState();
    g_NegconSteerPlay = 0;
    g_PadPressed = PAD_LEFT;
    UpdateNegconSteerPlayScreen();
    CHECK(g_NegconSteerPlay == 0 && s_soundCueCount == 0);

    ResetState();
    g_NegconMaxTwist = 3;
    g_PadPressed = PAD_RIGHT;
    UpdateNegconMaxTwistScreen();
    CHECK(g_NegconMaxTwist == 3 && s_soundCueCount == 0);

    ResetState();
    g_NegconSteerPlay = 2;
    g_PadPressed = PAD_LEFT | PAD_RIGHT;
    UpdateNegconSteerPlayScreen();
    CHECK(g_NegconSteerPlay == 2);
    CHECK(s_soundCueCount == 2 && s_soundCues[0] == 8 && s_soundCues[1] == 8);

    ResetState();
    g_NegconSteerPlay = 2;
    g_PadPressed = PAD_CANCEL | PAD_LEFT;
    UpdateNegconSteerPlayScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_NegconSteerPlay == 2);
    CHECK(s_restoreCount == 1);

    ResetState();
    g_NegconMaxTwist = 2;
    g_PadPressed = PAD_CONFIRM | PAD_RIGHT;
    UpdateNegconMaxTwistScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT && g_NegconMaxTwist == 2);
}

static void TestDisconnectRestoresSettings(void) {
    ResetState();
    g_PadType = PAD_TYPE_DIGITAL;
    UpdateNegconMaxTwistScreen();
    CHECK(g_GameMode == OPTION_MODE_ROOT);
    CHECK(s_restoreCount == 1);
    CheckSharedFrame(0, 1);
}

int main(void) {
    TestSteerNavigation();
    TestTwistNavigation();
    TestSettingRangeAndInputOrder();
    TestDisconnectRestoresSettings();
    return s_failures != 0;
}
