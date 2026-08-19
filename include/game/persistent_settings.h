#ifndef GAME_PERSISTENT_SETTINGS_H
#define GAME_PERSISTENT_SETTINGS_H

#include "common.h"

typedef struct PersistentSettings {
    s32 screenOffsetX;
    s32 screenOffsetY;
    s16 negconSteerPlay;
    s16 padMappingIndex;
    s16 negconMappingIndex;
    s16 negconSteerNeutral;
    s16 negconNeutralI;
    s16 negconNeutralII;
    s16 negconNeutralL;
    s16 negconMaxTwist;
    s32 padErrorState;
    s32 padValidateCountdown;
    s32 padErrorHoldBits;
    s32 mirrorMode;
    s16 extraGrandPrixUnlocked;
} PersistentSettings;

PersistentSettings PersistentSettingsDefaults(void);
void PersistentSettingsReset(PersistentSettings *settings);

#endif
