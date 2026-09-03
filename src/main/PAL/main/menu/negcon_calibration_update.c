#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/state.h"
#include "game/input_internal.h"

enum {
    CONTROLLER_SCENE_ANGLE_X = -896,
};

static void AdjustCalibrationValue(NegconCalibrationValue *value) {
    if ((g_PadPressed & PAD_LEFT) && *value > NEGCON_CALIBRATION_FIRST) {
        PlaySoundCue(8);
        (*value)--;
    }
    if ((g_PadPressed & PAD_RIGHT) && *value < NEGCON_CALIBRATION_LAST) {
        PlaySoundCue(8);
        (*value)++;
    }
}

static void FinishCalibrationFrame(void (*drawScreen)(void)) {
    if (g_PadType != PAD_TYPE_NEGCON) {
        g_GameMode = OPTION_MODE_ROOT;
        RestoreNegconCalibrationSettings();
    }
    g_ControllerSceneAngleX = CONTROLLER_SCENE_ANGLE_X;
    drawScreen();
    DrawOptionHintBar(4);
    DrawControllerSetupScene(1);
}

void UpdateNegconSteerPlayScreen(void) {
    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    g_SetupArrowPulse = (s32)((u32)g_SetupArrowPulse + 96u);
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_NEGCON_MAX_TWIST;
    } else {
        AdjustCalibrationValue(&g_NegconSteerPlay);
    }
    FinishCalibrationFrame(DrawNegconSteerPlayScreen);
}

void UpdateNegconMaxTwistScreen(void) {
    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_ROOT;
    } else {
        AdjustCalibrationValue(&g_NegconMaxTwist);
    }
    FinishCalibrationFrame(DrawNegconMaxTwistScreen);
}
