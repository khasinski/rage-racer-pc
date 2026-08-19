#include "game/boot_defaults.h"

enum { PAD_DEFAULT_VALIDATE_COUNTDOWN = 0x21 };

GameBootDefaults GameBootDefaultsCreate(void) {
    GameBootDefaults defaults = {0};
    defaults.input.negconSteerPlay = 1;
    defaults.padRuntime.errorState = PAD_ERROR_STATE_NONE;
    defaults.padRuntime.validateCountdown = PAD_DEFAULT_VALIDATE_COUNTDOWN;
    return defaults;
}

void GameBootDefaultsReset(GameBootDefaults *defaults) {
    *defaults = GameBootDefaultsCreate();
}
