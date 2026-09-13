#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/state.h"
#include "game/input_internal.h"

enum {
    CONTROLLER_SCENE_ANGLE_X = -896,
};

static void AdjustCalibrationValue(NegconCalibrationValue *value) {
    *value = (NegconCalibrationValue)NegconCalibrationIndex(*value);
    if ((g_PadPressed & PAD_LEFT) && *value > NEGCON_CALIBRATION_FIRST) {
        PlaySoundCue(8);
        (*value)--;
    }
    if ((g_PadPressed & PAD_RIGHT) && *value < NEGCON_CALIBRATION_LAST) {
        PlaySoundCue(8);
        (*value)++;
    }
}

static void FinishCalibrationFrame(ControllerSetup *setup,
                                   void (*drawScreen)(s32)) {
    setup->angleX = CONTROLLER_SCENE_ANGLE_X;
    drawScreen(setup->arrowPhase);
    DrawOptionHintBar(MENU_OPTION_HINT_NEGCON_CALIBRATION);
    DrawControllerSetupScene(setup, 1);
}

static int LeaveIfNegconDisconnected(void) {
    if (g_PadType == PAD_TYPE_NEGCON) {
        return 0;
    }
    g_GameMode = OPTION_MODE_ROOT;
    RestoreNegconCalibrationSettings();
    return 1;
}

void UpdateNegconSteerPlayScreen(void) {
    ControllerSetup *setup = MenuControllerSetup();

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    setup->arrowPhase = (s32)((u32)setup->arrowPhase + 96u);
    if (!LeaveIfNegconDisconnected()) {
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
    }
    FinishCalibrationFrame(setup, DrawNegconSteerPlayScreen);
}

void UpdateNegconMaxTwistScreen(void) {
    ControllerSetup *setup = MenuControllerSetup();

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    if (!LeaveIfNegconDisconnected()) {
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
    }
    FinishCalibrationFrame(setup, DrawNegconMaxTwistScreen);
}
