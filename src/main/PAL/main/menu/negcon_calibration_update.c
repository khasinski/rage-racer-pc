#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/state.h"
#include "game/input_internal.h"

enum {
    NEGCON_CALIBRATION_MIN = 0,
    NEGCON_CALIBRATION_MAX = 3,
    CONTROLLER_SCENE_ANGLE_X = -896,
};

static void AdjustCalibrationValue(NegconCalibrationValue *value) {
    if ((g_PadPressed & PAD_LEFT) && *value > NEGCON_CALIBRATION_MIN) {
        PlaySoundCue(8);
        (*value)--;
    }
    if ((g_PadPressed & PAD_RIGHT) && *value < NEGCON_CALIBRATION_MAX) {
        PlaySoundCue(8);
        (*value)++;
    }
}

static void FinishCalibrationFrame(void (*drawScreen)(void)) {
    if (g_PadType != PAD_TYPE_NEGCON) {
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    }
    g_ControllerSceneAngleX = CONTROLLER_SCENE_ANGLE_X;
    drawScreen();
    DrawOptionHintBar(4);
    DrawControllerSetupScene(1);
}

void UpdateNegconSteerPlayScreen(void) {
    g_AnimTimer++;
    g_SetupArrowPulse += 96;
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = 11;
    }
    AdjustCalibrationValue(&g_NegconSteerPlay);
    FinishCalibrationFrame(DrawNegconSteerPlayScreen);
}

void UpdateNegconMaxTwistScreen(void) {
    g_AnimTimer++;
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = 1;
    }
    AdjustCalibrationValue(&g_NegconMaxTwist);
    FinishCalibrationFrame(DrawNegconMaxTwistScreen);
}
