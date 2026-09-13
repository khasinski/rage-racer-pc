#include "game/audio.h"
#include "game/menu.h"
#include "game/input_internal.h"
#include "game/state.h"

typedef struct NegconCalibrationBackup {
    NegconCalibrationValue steerNeutral;
    NegconCalibrationValue neutralI;
    NegconCalibrationValue neutralII;
    NegconCalibrationValue neutralL;
    NegconCalibrationValue steerPlay;
    NegconCalibrationValue maxTwist;
} NegconCalibrationBackup;

static NegconCalibrationBackup s_backup;

void BeginNegconCalibration(void) {
    ControllerSetup *setup = MenuControllerSetup();

    s_backup = (NegconCalibrationBackup){
        .steerNeutral = g_NegconSteerNeutral,
        .neutralI = g_NegconNeutralI,
        .neutralII = g_NegconNeutralII,
        .neutralL = g_NegconNeutralL,
        .steerPlay = g_NegconSteerPlay,
        .maxTwist = g_NegconMaxTwist,
    };

    g_NegconSteerNeutral = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    setup->angleY = 0;
    setup->angleX = 0;
    g_GameMode = OPTION_MODE_NEGCON_NEUTRAL;
}

void RestoreNegconCalibrationSettings(void) {
    g_NegconSteerNeutral = s_backup.steerNeutral;
    g_NegconNeutralI = s_backup.neutralI;
    g_NegconNeutralII = s_backup.neutralII;
    g_NegconNeutralL = s_backup.neutralL;
    g_NegconSteerPlay = s_backup.steerPlay;
    g_NegconMaxTwist = s_backup.maxTwist;
}

void UpdateNegconNeutralScreen(void) {
    ControllerSetup *setup = MenuControllerSetup();

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    if (g_PadPressed & PAD_START) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_NEGCON_STEER_PLAY;
        g_NegconSteerNeutral = g_NegconAxisSteer - 128;
        g_NegconNeutralI = g_NegconAxisI;
        g_NegconNeutralII = g_NegconAxisII;
        g_NegconNeutralL = g_NegconAxisL;
    }
    if (g_PadType != PAD_TYPE_NEGCON) {
        RestoreNegconCalibrationSettings();
        g_GameMode = OPTION_MODE_ROOT;
    }
    DrawNegconNeutralScreen();
    DrawOptionHintBar(MENU_OPTION_HINT_NEGCON_CALIBRATION);
    DrawControllerSetupScene(setup, 0);
}
