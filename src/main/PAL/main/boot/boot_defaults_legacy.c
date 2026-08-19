#include "game/boot_defaults_legacy.h"
#include "game/boot_legacy_globals.h"

void GameBootDefaultsApplyLegacy(const GameBootDefaults *defaults) {
    g_ScreenOffsetX = defaults->display.screenOffsetX;
    g_ScreenOffsetY = defaults->display.screenOffsetY;
    g_MirrorMode = defaults->display.mirrorMode;
    g_NegconSteerPlay = defaults->input.negconSteerPlay;
    g_PadMappingIndex = defaults->input.padMappingIndex;
    g_NegconMappingIndex = defaults->input.negconMappingIndex;
    g_NegconSteerNeutral = defaults->input.negconSteerNeutral;
    g_NegconNeutralI = defaults->input.negconNeutralI;
    g_NegconNeutralII = defaults->input.negconNeutralII;
    g_NegconNeutralL = defaults->input.negconNeutralL;
    g_NegconMaxTwist = defaults->input.negconMaxTwist;
    g_PadErrorState = defaults->padRuntime.errorState;
    g_PadValidateCountdown = defaults->padRuntime.validateCountdown;
    g_PadErrorHoldBits = defaults->padRuntime.errorHoldBits;
    g_ExtraGrandPrixUnlocked = defaults->progress.extraGrandPrixUnlocked;
}

void GameBootDefaultsCaptureLegacy(GameBootDefaults *defaults) {
    defaults->display.screenOffsetX = g_ScreenOffsetX;
    defaults->display.screenOffsetY = g_ScreenOffsetY;
    defaults->display.mirrorMode = g_MirrorMode;
    defaults->input.negconSteerPlay = g_NegconSteerPlay;
    defaults->input.padMappingIndex = g_PadMappingIndex;
    defaults->input.negconMappingIndex = g_NegconMappingIndex;
    defaults->input.negconSteerNeutral = g_NegconSteerNeutral;
    defaults->input.negconNeutralI = g_NegconNeutralI;
    defaults->input.negconNeutralII = g_NegconNeutralII;
    defaults->input.negconNeutralL = g_NegconNeutralL;
    defaults->input.negconMaxTwist = g_NegconMaxTwist;
    defaults->padRuntime.errorState = g_PadErrorState;
    defaults->padRuntime.validateCountdown = g_PadValidateCountdown;
    defaults->padRuntime.errorHoldBits = g_PadErrorHoldBits;
    defaults->progress.extraGrandPrixUnlocked = g_ExtraGrandPrixUnlocked;
}
