#ifndef GAME_BOOT_DEFAULTS_H
#define GAME_BOOT_DEFAULTS_H

#include "common.h"
#include "game/input_internal.h"
#include "game/pad.h"

typedef struct DisplaySettings {
    s32 screenOffsetX;
    s32 screenOffsetY;
    s32 mirrorMode;
} DisplaySettings;

typedef struct InputSettings {
    NegconCalibrationValue negconSteerPlay;
    ControllerMappingIndex padMappingIndex;
    ControllerMappingIndex negconMappingIndex;
    NegconCalibrationValue negconSteerNeutral;
    NegconCalibrationValue negconNeutralI;
    NegconCalibrationValue negconNeutralII;
    NegconCalibrationValue negconNeutralL;
    NegconCalibrationValue negconMaxTwist;
} InputSettings;

typedef struct PadRuntimeState {
    PadErrorState errorState;
    s32 validateCountdown;
    s32 errorHoldBits;
} PadRuntimeState;

typedef struct ProgressDefaults {
    s16 extraGrandPrixUnlocked;
} ProgressDefaults;

typedef struct GameBootDefaults {
    DisplaySettings display;
    InputSettings input;
    PadRuntimeState padRuntime;
    ProgressDefaults progress;
} GameBootDefaults;

GameBootDefaults GameBootDefaultsCreate(void);
void GameBootDefaultsReset(GameBootDefaults *defaults);

#endif
