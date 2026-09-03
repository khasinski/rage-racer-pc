#include "game/audio.h"
#include "game/menu.h"
#include "game/input_internal.h"
#include "game/state.h"

void BeginNegconCalibration(void) {
    g_NegconSteerNeutralSaved = g_NegconSteerNeutral;
    g_NegconNeutralISaved = g_NegconNeutralI;
    g_NegconNeutralIISaved = g_NegconNeutralII;
    g_NegconNeutralLSaved = g_NegconNeutralL;
    g_NegconSteerPlaySaved = g_NegconSteerPlay;
    g_NegconMaxTwistSaved = g_NegconMaxTwist;

    g_NegconSteerNeutral = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_ControllerSceneAngleY = 0;
    g_ControllerSceneAngleX = 0;
    g_PadConfigFlipTimer = 0;
    g_PadConfigFlipPhase = 0;
    g_GameMode = OPTION_MODE_NEGCON_NEUTRAL;
}

void RestoreNegconCalibrationSettings(void) {
    g_NegconSteerNeutral = g_NegconSteerNeutralSaved;
    g_NegconNeutralI = g_NegconNeutralISaved;
    g_NegconNeutralII = g_NegconNeutralIISaved;
    g_NegconNeutralL = g_NegconNeutralLSaved;
    g_NegconSteerPlay = g_NegconSteerPlaySaved;
    g_NegconMaxTwist = g_NegconMaxTwistSaved;
}

void UpdateNegconNeutralScreen(void) {
    g_AnimTimer++;
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
    DrawOptionHintBar(4);
    DrawControllerSetupScene(0);
}
