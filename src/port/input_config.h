#ifndef RAGE_INPUT_CONFIG_H
#define RAGE_INPUT_CONFIG_H

#include <stddef.h>

enum { RAGE_INPUT_BUTTON_COUNT = 16, RAGE_INPUT_KEY_NAME_MAX = 31 };

typedef struct RageInputConfig {
    char keys[RAGE_INPUT_BUTTON_COUNT][RAGE_INPUT_KEY_NAME_MAX + 1];
} RageInputConfig;

void RageInputConfigDefaults(RageInputConfig *config);
int RageInputConfigLoad(RageInputConfig *config, const char *path);
int RageInputConfigApplyRuntime(RageInputConfig *config);
int RageInputButtonIndex(const char *name);

#endif
