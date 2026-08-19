#include "game/persistent_settings.h"

PersistentSettings PersistentSettingsDefaults(void) {
    PersistentSettings settings = {0};
    settings.negconSteerPlay = 1;
    settings.padValidateCountdown = 0x21;
    return settings;
}

void PersistentSettingsReset(PersistentSettings *settings) {
    *settings = PersistentSettingsDefaults();
}
